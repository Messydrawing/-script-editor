"""Collection of undoable commands for node and scene manipulation."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Dict, List, Sequence

from PySide6.QtCore import QPointF
from PySide6.QtGui import QColor, QUndoCommand

from gui.edge_item import EdgeItem
from gui.node_item import NodeItem
from gui.scene_view import GraphScene
from model.document_manager import DocumentManager


@dataclass
class _StoredNode:
    node_id: int
    summary: str
    doc_path: str
    position: QPointF
    color: QColor


@dataclass
class _StoredEdge:
    source_id: int
    dest_id: int
    condition: str


class AddNodeCommand(QUndoCommand):
    """Command that inserts a node into the scene."""

    def __init__(
        self,
        scene: GraphScene,
        node_id: int,
        summary: str,
        doc_path: str,
        position: QPointF,
    ) -> None:
        super().__init__("Add Node")
        self._scene = scene
        self._node = _StoredNode(
            node_id=node_id,
            summary=summary,
            doc_path=doc_path,
            position=QPointF(position),
            color=QColor("#87CEFA"),
        )
        self._created_item: NodeItem | None = None

    # ------------------------------------------------------------------
    def redo(self) -> None:  # type: ignore[override]
        DocumentManager.ensure_document(
            self._node.doc_path,
            title=self._node.summary,
            summary=self._node.summary,
        )
        item = self._scene.create_node(
            node_id=self._node.node_id,
            summary=self._node.summary,
            doc_path=self._node.doc_path,
            position=self._node.position,
        )
        item.setBrush(self._node.color)
        self._created_item = item

    # ------------------------------------------------------------------
    def undo(self) -> None:  # type: ignore[override]
        if self._created_item is None:
            # The node might have been recreated after an undo/redo cycle.
            self._created_item = self._scene.find_node(self._node.node_id)
        if self._created_item is not None:
            self._scene.remove_item(self._created_item)
            self._created_item = None


class DeleteItemsCommand(QUndoCommand):
    """Remove selected nodes and edges while supporting undo."""

    def __init__(self, scene: GraphScene, items: Sequence[object]) -> None:
        label = "Delete Items"
        super().__init__(label)
        self._scene = scene

        node_items: List[NodeItem] = []
        edge_items: List[EdgeItem] = []
        for item in items:
            if isinstance(item, NodeItem):
                node_items.append(item)
            elif isinstance(item, EdgeItem):
                edge_items.append(item)

        self._nodes: List[_StoredNode] = [
            _StoredNode(
                node_id=node.node_id,
                summary=node.summary,
                doc_path=node.doc_path,
                position=QPointF(node.pos()),
                color=node.brush().color(),
            )
            for node in node_items
        ]

        seen_edges: set[tuple[int, int, str]] = set()
        stored_edges: List[_StoredEdge] = []

        def _record_edge(edge: EdgeItem) -> None:
            key = (edge.source.node_id, edge.dest.node_id, edge.text_item.toPlainText())
            if key in seen_edges:
                return
            seen_edges.add(key)
            stored_edges.append(
                _StoredEdge(
                    source_id=edge.source.node_id,
                    dest_id=edge.dest.node_id,
                    condition=edge.text_item.toPlainText(),
                )
            )

        for edge in edge_items:
            _record_edge(edge)

        for node in node_items:
            for edge in list(node.edges):
                _record_edge(edge)

        self._edges = stored_edges

    # ------------------------------------------------------------------
    def redo(self) -> None:  # type: ignore[override]
        # Remove edges first so that we do not leave dangling references.
        for edge in self._edges:
            edge_item = self._scene.find_edge(edge.source_id, edge.dest_id, edge.condition)
            if edge_item is not None:
                edge_item.remove()
        for stored in self._nodes:
            node_item = self._scene.find_node(stored.node_id)
            if node_item is not None:
                self._scene.remove_item(node_item)

    # ------------------------------------------------------------------
    def undo(self) -> None:  # type: ignore[override]
        for stored in self._nodes:
            DocumentManager.ensure_document(
                stored.doc_path,
                title=stored.summary,
                summary=stored.summary,
            )
            node = self._scene.create_node(
                node_id=stored.node_id,
                summary=stored.summary,
                doc_path=stored.doc_path,
                position=stored.position,
            )
            node.setBrush(stored.color)
        # After nodes are restored, rebuild the edges.
        for edge in self._edges:
            source = self._scene.find_node(edge.source_id)
            dest = self._scene.find_node(edge.dest_id)
            if source is None or dest is None:
                continue
            new_edge = EdgeItem(source, dest, condition=edge.condition)
            self._scene.addItem(new_edge)
            new_edge.adjust()


class MoveNodesCommand(QUndoCommand):
    """Command encapsulating a group of node movements."""

    def __init__(
        self,
        scene: GraphScene,
        before: Dict[int, QPointF],
        after: Dict[int, QPointF],
    ) -> None:
        super().__init__("Move Nodes")
        self._scene = scene
        self._before = before
        self._after = after

    # ------------------------------------------------------------------
    def _apply_positions(self, positions: Dict[int, QPointF]) -> None:
        for node_id, pos in positions.items():
            node = self._scene.find_node(node_id)
            if node is not None:
                node.setPos(pos)

    # ------------------------------------------------------------------
    def redo(self) -> None:  # type: ignore[override]
        self._apply_positions(self._after)

    # ------------------------------------------------------------------
    def undo(self) -> None:  # type: ignore[override]
        self._apply_positions(self._before)


class UpdateNodeSummaryCommand(QUndoCommand):
    """Update the textual summary shown on a node."""

    def __init__(self, scene: GraphScene, node_id: int, before: str, after: str) -> None:
        super().__init__("Edit Summary")
        self._scene = scene
        self._node_id = node_id
        self._before = before
        self._after = after

    # ------------------------------------------------------------------
    def redo(self) -> None:  # type: ignore[override]
        node = self._scene.find_node(self._node_id)
        if node is not None:
            node.update_summary(self._after)

    # ------------------------------------------------------------------
    def undo(self) -> None:  # type: ignore[override]
        node = self._scene.find_node(self._node_id)
        if node is not None:
            node.update_summary(self._before)
