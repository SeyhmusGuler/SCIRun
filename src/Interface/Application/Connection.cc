/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <stdexcept>
#include <QtGui>
#include <boost/bind.hpp>
#include <Interface/Application/Connection.h>
#include <Interface/Application/Utility.h>
#include <Interface/Application/Port.h>
#include <Interface/Application/GuiLogger.h>

using namespace SCIRun::Gui;

class EuclideanDrawStrategy : public ConnectionDrawStrategy
{
public:
  void draw(QGraphicsPathItem* item, const QPointF& from, const QPointF& to)
  {
    QPainterPath path;
    path.moveTo(from);
    path.lineTo(to);
    item->setPath(path);
  }
};

class CubicBezierDrawStrategy : public ConnectionDrawStrategy
{
public:
  void draw(QGraphicsPathItem* item, const QPointF& from, const QPointF& to)
  {
    QPainterPath path;
    QPointF start = from;

    path.moveTo(start);
    auto mid = (to - start) / 2 + start;

    QPointF qDir(-(to-start).y() / ((double)(to-start).x()) , 1);
    double qFactor = std::min(std::abs(100.0 / qDir.x()), 80.0);
    //TODO: scale down when start close to end. need a unit test at this point.
    //qFactor /= (end-start).manhattanDistance()

    auto q1 = item->mapToScene(mid + qFactor * qDir);
    auto q2 = item->mapToScene(mid - qFactor * qDir);
    path.cubicTo(q1, q2, to);  
    item->setPath(path);
  }
};


class ManhattanDrawStrategy : public ConnectionDrawStrategy
{
public:
  void draw(QGraphicsPathItem* item, const QPointF& from, const QPointF& to)
  {
    QPainterPath path;
    path.moveTo(from);
    const int case1Threshold = 15;
    if (from.y() > to.y() - case1Threshold) // input above output
    {
      path.lineTo(from.x(), from.y() + case1Threshold);
      const int leftSideBuffer = 30;
      auto nextX = std::min(from.x() - leftSideBuffer, to.x() - leftSideBuffer); // TODO will be a function of port position
      path.lineTo(nextX, from.y() + case1Threshold); // TODO will be a function of port position
      path.lineTo(nextX, to.y() - case1Threshold);
      path.lineTo(to.x(), to.y() - case1Threshold);
      path.lineTo(to);
    }
    else  // output above input
    {
      auto midY = (from.y() + to.y()) / 2;
      path.lineTo(from.x(), midY);
      path.lineTo(to.x(), midY);
      path.lineTo(to);
    }
    item->setPath(path);
  }
};

ConnectionLine::ConnectionLine(PortWidget* fromPort, PortWidget* toPort, const SCIRun::Dataflow::Networks::ConnectionId& id, ConnectionDrawStrategyPtr drawer)
  : fromPort_(fromPort), toPort_(toPort), id_(id), destroyed_(false), drawer_(drawer)
{
  if (fromPort_)
  {
    fromPort_->addConnection(this);
    fromPort_->turn_on_light();
  }
  else
    std::cout << "NULL FROM PORT: " << id_.id_ << std::endl;
  if (toPort_)
  {
    toPort_->addConnection(this);
    toPort_->turn_on_light();
  }
  else
    std::cout << "NULL TO PORT: " << id_.id_ << std::endl;

  if (fromPort_ && toPort_)
    setColor(fromPort_->color());

  setFlags(QGraphicsItem::ItemIsSelectable);
  //TODO: need dynamic zValue
  setZValue(1); 
  setToolTip("Left - Highlight*\nDouble-Left - Menu");

  trackNodes();
  GuiLogger::Instance().log("Connection made.");
}

ConnectionLine::~ConnectionLine()
{
  if (!destroyed_)
    destroy();
}

void ConnectionLine::destroy() 
{
  if (fromPort_ && toPort_)
  {
    fromPort_->removeConnection(this);
    fromPort_->turn_off_light();
    toPort_->removeConnection(this);
    toPort_->turn_off_light();
  }
  drawer_.reset();
  Q_EMIT deleted(id_);
  GuiLogger::Instance().log("Connection deleted.");
  destroyed_ = true;
}

void ConnectionLine::setColor(const QColor& color)
{
  setPen(QPen(color, 5.0));
}

QColor ConnectionLine::color() const
{
  return pen().color();
}

void ConnectionLine::trackNodes()
{
  //std::cout << "trackNodes " << id_.id_ << std::endl;
  if (fromPort_ && toPort_)
  {
    drawer_->draw(this, fromPort_->position(), toPort_->position());
  }
  else
    BOOST_THROW_EXCEPTION(InvalidConnection() << Core::ErrorMessage("no from/to set for Connection: " + id_.id_));
}

void ConnectionLine::setDrawStrategy(ConnectionDrawStrategyPtr cds)
{
  if (!destroyed_)
  {
    drawer_ = cds;
    trackNodes();
  }
}

namespace
{
  const QString deleteAction("Delete");
  const QString insertModuleAction("Insert Module->*");
  const QString disableEnableAction("Disable*");
  const QString editNotesAction("Edit Notes...*");
}

class ConnectionMenu : public QMenu
{
public:
  ConnectionMenu()
  {
    addAction(deleteAction);
    addAction(insertModuleAction)->setDisabled(true);
    addAction(disableEnableAction)->setDisabled(true);
    addAction(editNotesAction)->setDisabled(true);
  }
};

void ConnectionLine::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  ConnectionMenu menu;
  auto a = menu.exec(event->screenPos());
  if (a && a->text() == deleteAction)
  {
    scene()->removeItem(this);
    destroy();
  }
}

ConnectionInProgressStraight::ConnectionInProgressStraight(PortWidget* port, ConnectionDrawStrategyPtr drawer)
  : ConnectionInProgressGraphicsItem<QGraphicsLineItem>(port, drawer)
{
}

void ConnectionInProgressStraight::update(const QPointF& end)
{
  //TODO: use strategy object. probably need to improve first parameter: templatized? or just change this case to use QGraphicsPathItem directly
  //drawStrategy_->draw(this, fromPort_->position(), end);
  
  setLine(QLineF(fromPort_->position(), end));
}

ConnectionInProgressCurved::ConnectionInProgressCurved(PortWidget* port, ConnectionDrawStrategyPtr drawer)
  : ConnectionInProgressGraphicsItem<QGraphicsPathItem>(port, drawer)
{
}

void ConnectionInProgressCurved::update(const QPointF& end)
{
  drawStrategy_->draw(this, fromPort_->position(), end);
}

ConnectionInProgressManhattan::ConnectionInProgressManhattan(PortWidget* port, ConnectionDrawStrategyPtr drawer)
  : ConnectionInProgressGraphicsItem<QGraphicsPathItem>(port, drawer)
{
}

void ConnectionInProgressManhattan::update(const QPointF& end)
{
  if (fromPort_->isInput())
    drawStrategy_->draw(this, end, fromPort_->position());
  else
    drawStrategy_->draw(this, fromPort_->position(), end);
}


ConnectionFactory::ConnectionFactory(QGraphicsScene* scene) : currentType_(EUCLIDEAN), scene_(scene), 
  euclidean_(new EuclideanDrawStrategy), 
  cubic_(new CubicBezierDrawStrategy),
  manhattan_(new ManhattanDrawStrategy)
{}

ConnectionInProgress* ConnectionFactory::makeConnectionInProgress(PortWidget* port) const
{
  switch (currentType_)
  {
    case EUCLIDEAN:
    {
      auto c = new ConnectionInProgressStraight(port, getCurrentDrawer());
      activate(c);
      return c;
    }
    case CUBIC:
    {
      auto c = new ConnectionInProgressCurved(port, getCurrentDrawer());
      activate(c);
      return c;
    }
    case MANHATTAN:
    {
      auto c = new ConnectionInProgressManhattan(port, getCurrentDrawer());
      activate(c);
      return c;
    }
    default:
      std::cerr << "Unknown connection type." << std::endl;
      return 0;
  }
}

void ConnectionFactory::activate(QGraphicsItem* item) const
{
  if (item)
  {
    if (scene_)
      scene_->addItem(item);
    item->setVisible(true);
  }
}

void ConnectionFactory::setType(ConnectionDrawType type)
{
  if (type != currentType_)
  {
    currentType_ = type;
    Q_EMIT typeChanged(getCurrentDrawer());
  }
}

ConnectionDrawType ConnectionFactory::getType() const
{
  return currentType_;
}

ConnectionDrawStrategyPtr ConnectionFactory::getCurrentDrawer() const
{
  switch (currentType_)
  {
  case EUCLIDEAN:
    return euclidean_;
  case CUBIC:
    return cubic_;
  case MANHATTAN:
    return manhattan_;
  default:
    std::cerr << "Unknown connection type." << std::endl;
    return ConnectionDrawStrategyPtr();
  }
}

ConnectionLine* ConnectionFactory::makeFinishedConnection(PortWidget* fromPort, PortWidget* toPort, const SCIRun::Dataflow::Networks::ConnectionId& id) const
{
  auto c = new ConnectionLine(fromPort, toPort, id, getCurrentDrawer());
  activate(c);
  connect(this, SIGNAL(typeChanged(ConnectionDrawStrategyPtr)), c, SLOT(setDrawStrategy(ConnectionDrawStrategyPtr)));
  return c;
}