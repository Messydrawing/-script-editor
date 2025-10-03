"""Scene and view classes for the screenplay graph editor."""
from __future__ import annotations

from enum import Enum, auto
from typing import Iterable, Optional

from PySide6.QtCore import QPointF, QRectF, Signal
from PySide6.QtGui import QPainter, QTransform
from PySide6.QtWidgets import QGraphicsScene, QGraphicsView

from gui.edge_item import EdgeItem
from gui.node_item import NodeItem


class GraphMode(Enum):
    POINTER = auto()
    ADD_EDGE = auto()


class GraphScene(QGraphicsScene):
    """Scene responsible for managing nodes and edges."""

    nodeInserted = Signal(NodeItem)
    edgeInserted = Signal(EdgeItem)

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self._mode = GraphMode.POINTER
        self._edge_start: Optional[NodeItem] = None

    # -- scene utilities -------------------------------------------------
    def set_mode(self, mode: GraphMode) -> None:
        self._mode = mode
        if mode != GraphMode.ADD_EDGE:
            self._edge_start = None

    def mode(self) -> GraphMode:
        return self._mode

    def create_node(self, node_id: int, summary: str, doc_path: str, position: Optional[QPointF] = None) -> NodeItem:
        node = NodeItem(node_id=node_id, summary=summary, doc_path=doc_path)
        if position is not None:
            node.setPos(position)
        self.addItem(node)
        self.nodeInserted.emit(node)
        return node

    def nodes(self) -> Iterable[NodeItem]:
        for item in self.items():
            if isinstance(item, NodeItem):
                yield item

    def edges(self) -> Iterable[EdgeItem]:
        for item in self.items():
            if isinstance(item, EdgeItem):
                yield item

    def remove_item(self, item) -> None:
        if isinstance(item, EdgeItem):
            item.remove()
        elif isinstance(item, NodeItem):
            for edge in list(item.edges):
                edge.remove()
            self.removeItem(item)

    def remove_selection(self) -> None:
        for item in list(self.selectedItems()):
            self.remove_item(item)

    # -- Qt events -------------------------------------------------------
    def mousePressEvent(self, event):  # type: ignore[override]
        if self._mode == GraphMode.ADD_EDGE:
            item = self.itemAt(event.scenePos(), QTransform())
            if isinstance(item, NodeItem):
                if self._edge_start is None:
                    self._edge_start = item
                else:
                    if item is not self._edge_start:
                        edge = EdgeItem(self._edge_start, item)
                        self.addItem(edge)
                        self.edgeInserted.emit(edge)
                    self._edge_start = None
                event.accept()
                return
            else:
                self._edge_start = None
        super().mousePressEvent(event)


class GraphView(QGraphicsView):
    """Customized view that offers zooming conveniences."""

    def __init__(self, scene: GraphScene, parent=None) -> None:
        super().__init__(scene, parent)
        self.setRenderHints(self.renderHints() | QPainter.RenderHint.Antialiasing)
        self.setDragMode(self.DragMode.RubberBandDrag)
        self.setViewportUpdateMode(self.ViewportUpdateMode.BoundingRectViewportUpdate)

    def wheelEvent(self, event):  # type: ignore[override]
        zoom_in_factor = 1.15
        zoom_out_factor = 1 / zoom_in_factor
        if event.angleDelta().y() > 0:
            factor = zoom_in_factor
        else:
            factor = zoom_out_factor
        self.scale(factor, factor)

    def ensure_visible(self, rect: QRectF) -> None:
        self.ensureVisible(rect)
