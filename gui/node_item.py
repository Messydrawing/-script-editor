"""Graphics item representing a visual novel node."""
from __future__ import annotations

import os
from dataclasses import dataclass
from typing import List, Optional

from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QBrush, QColor, QPen
from PySide6.QtWidgets import QGraphicsRectItem, QGraphicsTextItem

from model.document_manager import DocumentManager


@dataclass
class NodeData:
    """Serializable description of a node."""

    node_id: int
    summary: str
    doc_path: str
    position: QPointF
    color: QColor


class NodeItem(QGraphicsRectItem):
    """A draggable node linked to a screenplay document."""

    WIDTH = 160
    HEIGHT = 100

    def __init__(self, node_id: int, summary: str, doc_path: str, color: Optional[QColor] = None) -> None:
        super().__init__(-self.WIDTH / 2, -self.HEIGHT / 2, self.WIDTH, self.HEIGHT)
        self.node_id = node_id
        self.summary = summary
        self.doc_path = doc_path
        self.edges: List["EdgeItem"] = []
        self.setPen(QPen(QColor("#444"), 2))
        self.setBrush(QBrush(color or QColor("#87CEFA")))
        self.setFlag(QGraphicsRectItem.GraphicsItemFlag.ItemIsMovable, True)
        self.setFlag(QGraphicsRectItem.GraphicsItemFlag.ItemIsSelectable, True)
        self.setFlag(QGraphicsRectItem.GraphicsItemFlag.ItemSendsGeometryChanges, True)
        self.setAcceptHoverEvents(True)

        self._text_item = QGraphicsTextItem(summary, self)
        self._text_item.setDefaultTextColor(Qt.black)
        self._text_item.setTextWidth(self.WIDTH - 20)
        self._text_item.setPos(-self.WIDTH / 2 + 10, -self.HEIGHT / 2 + 10)

    # -- public API -----------------------------------------------------
    def add_edge(self, edge: "EdgeItem") -> None:
        if edge not in self.edges:
            self.edges.append(edge)

    def remove_edge(self, edge: "EdgeItem") -> None:
        if edge in self.edges:
            self.edges.remove(edge)

    def update_summary(self, summary: str) -> None:
        self.summary = summary
        self._text_item.setPlainText(summary)

    def to_data(self) -> NodeData:
        return NodeData(
            node_id=self.node_id,
            summary=self.summary,
            doc_path=self.doc_path,
            position=self.scenePos(),
            color=self.brush().color(),
        )

    # -- Qt events ------------------------------------------------------
    def mouseDoubleClickEvent(self, event):  # type: ignore[override]
        if os.path.exists(self.doc_path):
            DocumentManager.open_doc(self.doc_path)
        else:
            DocumentManager.create_document(self.doc_path, title=f"Node {self.node_id}")
            DocumentManager.open_doc(self.doc_path)
        super().mouseDoubleClickEvent(event)

    def itemChange(self, change, value):  # type: ignore[override]
        if change == QGraphicsRectItem.GraphicsItemChange.ItemPositionHasChanged:
            for edge in list(self.edges):
                edge.adjust()
        return super().itemChange(change, value)
