"""Undo commands built on Qt's undo framework."""
from __future__ import annotations

from PySide6.QtCore import QPointF
from PySide6.QtGui import QUndoCommand

from model.document_manager import DocumentManager


class AddNodeCommand(QUndoCommand):
    """Command that inserts a node into the scene."""

    def __init__(self, scene, node_id: int, summary: str, doc_path: str, position: QPointF) -> None:
        super().__init__("Add Node")
        self.scene = scene
        self.node_id = node_id
        self.summary = summary
        self.doc_path = doc_path
        self.position = position
        self._node = None

    def redo(self) -> None:  # type: ignore[override]
        DocumentManager.ensure_document(self.doc_path, title=self.summary, summary=self.summary)
        node = self.scene.create_node(self.node_id, self.summary, self.doc_path)
        node.setPos(self.position)
        self._node = node

    def undo(self) -> None:  # type: ignore[override]
        if self._node is not None:
            self.scene.remove_item(self._node)
            self._node = None
