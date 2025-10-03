"""Graphics item representing a directional edge between nodes."""
from __future__ import annotations

import math
from typing import Optional

from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QBrush, QColor, QPainter, QPen, QPolygonF
from PySide6.QtWidgets import QGraphicsLineItem, QGraphicsTextItem


class EdgeItem(QGraphicsLineItem):
    """A line with an arrow that connects two nodes."""

    def __init__(self, source_node, dest_node, condition: str = "") -> None:
        super().__init__()
        self.source = source_node
        self.dest = dest_node
        self.condition_text = condition
        pen = QPen(QColor("#222"))
        pen.setWidth(2)
        self.setPen(pen)
        self.setZValue(-1)  # Keep behind nodes
        self.arrow_size = 12

        self.text_item = QGraphicsTextItem(condition, parent=self)
        self.text_item.setDefaultTextColor(Qt.black)
        self.text_item.setTextInteractionFlags(Qt.TextInteractionFlag.TextEditorInteraction)

        self.source.add_edge(self)
        self.dest.add_edge(self)
        self.adjust()

    # -- helpers ---------------------------------------------------------
    def mid_point(self) -> QPointF:
        line = self.line()
        return QPointF((line.x1() + line.x2()) / 2.0, (line.y1() + line.y2()) / 2.0)

    def adjust(self) -> None:
        source_point = self.source.scenePos()
        dest_point = self.dest.scenePos()
        self.setLine(source_point.x(), source_point.y(), dest_point.x(), dest_point.y())
        mid = self.mid_point()
        self.text_item.setPos(mid - QPointF(self.text_item.boundingRect().width() / 2, self.text_item.boundingRect().height() / 2))

    def to_dict(self) -> dict:
        return {
            "from": self.source.node_id,
            "to": self.dest.node_id,
            "condition": self.text_item.toPlainText(),
        }

    # -- Qt events -------------------------------------------------------
    def paint(self, painter: QPainter, option, widget: Optional[object] = None) -> None:  # type: ignore[override]
        super().paint(painter, option, widget)
        line = self.line()
        if line.length() == 0.0:
            return

        angle = math.atan2(-(line.dy()), line.dx())

        dest_point = QPointF(line.x2(), line.y2())
        arrow_p1 = dest_point + QPointF(math.sin(angle - math.pi / 3) * self.arrow_size,
                                        math.cos(angle - math.pi / 3) * self.arrow_size)
        arrow_p2 = dest_point + QPointF(math.sin(angle - math.pi + math.pi / 3) * self.arrow_size,
                                        math.cos(angle - math.pi + math.pi / 3) * self.arrow_size)

        arrow_head = QPolygonF([dest_point, arrow_p1, arrow_p2])
        painter.setBrush(QBrush(self.pen().color()))
        painter.drawPolygon(arrow_head)

    def remove(self) -> None:
        self.source.remove_edge(self)
        self.dest.remove_edge(self)
        scene = self.scene()
        if scene is not None:
            scene.removeItem(self)
