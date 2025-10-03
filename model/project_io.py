"""Serialization helpers for saving/loading projects."""
from __future__ import annotations

import json
from pathlib import Path
from typing import Dict, List

from PySide6.QtGui import QColor

from gui.edge_item import EdgeItem
from gui.node_item import NodeItem
from gui.scene_view import GraphScene
from model.document_manager import DocumentManager


class ProjectIO:
    """Persist and restore graph scenes to JSON files."""

    @staticmethod
    def save_project(scene: GraphScene, filename: str | Path) -> None:
        data: Dict[str, List[dict]] = {"nodes": [], "edges": []}
        for node in scene.nodes():
            node_data = node.to_data()
            data["nodes"].append(
                {
                    "id": node_data.node_id,
                    "summary": node_data.summary,
                    "doc": str(Path(node_data.doc_path).as_posix()),
                    "x": node_data.position.x(),
                    "y": node_data.position.y(),
                    "color": node_data.color.name(),
                }
            )
        for edge in scene.edges():
            data["edges"].append(edge.to_dict())

        path = Path(filename)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("w", encoding="utf-8") as fh:
            json.dump(data, fh, ensure_ascii=False, indent=2)

    @staticmethod
    def load_project(scene: GraphScene, filename: str | Path) -> int:
        path = Path(filename)
        if not path.exists():
            raise FileNotFoundError(path)

        with path.open("r", encoding="utf-8") as fh:
            data = json.load(fh)

        scene.clear()

        id_map: Dict[int, NodeItem] = {}
        max_id = 0
        for node_data in data.get("nodes", []):
            node_id = int(node_data["id"])
            max_id = max(max_id, node_id)
            doc_path = node_data.get("doc", f"docs/node_{node_id}.docx")
            summary = node_data.get("summary", f"Node {node_id}")
            DocumentManager.ensure_document(doc_path, title=summary)
            node = scene.create_node(node_id=node_id, summary=summary, doc_path=doc_path)
            node.setPos(node_data.get("x", 0.0), node_data.get("y", 0.0))
            node.setBrush(QColor(node_data.get("color", "#87CEFA")))
            id_map[node_id] = node

        for edge_data in data.get("edges", []):
            source_id = int(edge_data["from"])
            dest_id = int(edge_data["to"])
            if source_id not in id_map or dest_id not in id_map:
                continue
            edge = EdgeItem(id_map[source_id], id_map[dest_id], condition=edge_data.get("condition", ""))
            scene.addItem(edge)
            edge.adjust()

        return max_id
