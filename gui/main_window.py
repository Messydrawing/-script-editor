"""Main window hosting the screenplay graph editor."""
from __future__ import annotations

from pathlib import Path
from typing import Optional

from PySide6.QtCore import QPointF, Qt
from PySide6.QtGui import QAction, QActionGroup, QUndoStack
from PySide6.QtWidgets import (
    QFileDialog,
    QInputDialog,
    QMainWindow,
    QMessageBox,
    QDockWidget,
    QToolBar,
    QUndoView,
)

from gui.scene_view import GraphMode, GraphScene, GraphView
from model.project_io import ProjectIO
from src.gui.commands import AddNodeCommand, DeleteItemsCommand


class MainWindow(QMainWindow):
    """Top-level window orchestrating the editor UI."""

    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Visual Novel Script Editor")
        self.resize(1200, 800)

        self.docs_dir = Path("docs")
        self.docs_dir.mkdir(exist_ok=True)

        self._project_path: Optional[Path] = None
        self._next_node_id = 1

        self.undo_stack = QUndoStack(self)

        self.scene = GraphScene(self)
        self.scene.set_undo_stack(self.undo_stack)
        self.view = GraphView(self.scene, self)
        self.setCentralWidget(self.view)

        self.undo_stack.indexChanged.connect(self._refresh_scene_state)

        self._create_actions()
        self._create_menus()
        self._create_toolbars()
        self.statusBar().showMessage("Ready")

    # ------------------------------------------------------------------
    def _create_actions(self) -> None:
        self.new_project_action = QAction("New Project", self)
        self.new_project_action.triggered.connect(self.new_project)

        self.open_project_action = QAction("Open Project", self)
        self.open_project_action.triggered.connect(self.open_project)

        self.save_project_action = QAction("Save", self)
        self.save_project_action.triggered.connect(self.save_project)

        self.save_project_as_action = QAction("Save As…", self)
        self.save_project_as_action.triggered.connect(self.save_project_as)

        self.exit_action = QAction("Exit", self)
        self.exit_action.triggered.connect(self.close)

        self.add_node_action = QAction("Add Node", self)
        self.add_node_action.setShortcut("Ctrl+N")
        self.add_node_action.triggered.connect(self.add_node)

        self.delete_selection_action = QAction("Delete Selection", self)
        self.delete_selection_action.setShortcut("Delete")
        self.delete_selection_action.triggered.connect(self.delete_selection)

        self.pointer_mode_action = QAction("Pointer", self)
        self.pointer_mode_action.setCheckable(True)
        self.pointer_mode_action.triggered.connect(lambda: self._set_mode(GraphMode.POINTER))

        self.edge_mode_action = QAction("Connect Nodes", self)
        self.edge_mode_action.setCheckable(True)
        self.edge_mode_action.triggered.connect(lambda: self._set_mode(GraphMode.ADD_EDGE))

        self.mode_group = QActionGroup(self)
        self.mode_group.addAction(self.pointer_mode_action)
        self.mode_group.addAction(self.edge_mode_action)
        self.pointer_mode_action.setChecked(True)

        self.undo_action = self.undo_stack.createUndoAction(self, "Undo")
        self.undo_action.setShortcut("Ctrl+Z")
        self.redo_action = self.undo_stack.createRedoAction(self, "Redo")
        self.redo_action.setShortcut("Ctrl+Y")

        self.toggle_history_action = QAction("History", self)
        self.toggle_history_action.setCheckable(True)
        self.toggle_history_action.setChecked(False)
        self.toggle_history_action.triggered.connect(self._toggle_history_dock)

    def _create_menus(self) -> None:
        file_menu = self.menuBar().addMenu("&File")
        file_menu.addAction(self.new_project_action)
        file_menu.addAction(self.open_project_action)
        file_menu.addSeparator()
        file_menu.addAction(self.save_project_action)
        file_menu.addAction(self.save_project_as_action)
        file_menu.addSeparator()
        file_menu.addAction(self.exit_action)

        edit_menu = self.menuBar().addMenu("&Edit")
        edit_menu.addAction(self.undo_action)
        edit_menu.addAction(self.redo_action)
        edit_menu.addSeparator()
        edit_menu.addAction(self.add_node_action)
        edit_menu.addAction(self.delete_selection_action)
        edit_menu.addSeparator()
        edit_menu.addAction(self.toggle_history_action)

        mode_menu = self.menuBar().addMenu("&Mode")
        mode_menu.addAction(self.pointer_mode_action)
        mode_menu.addAction(self.edge_mode_action)

    def _create_toolbars(self) -> None:
        toolbar = QToolBar("Main", self)
        toolbar.addAction(self.add_node_action)
        toolbar.addAction(self.delete_selection_action)
        toolbar.addSeparator()
        toolbar.addActions([self.pointer_mode_action, self.edge_mode_action])
        toolbar.addSeparator()
        toolbar.addAction(self.undo_action)
        toolbar.addAction(self.redo_action)
        toolbar.addAction(self.toggle_history_action)
        self.addToolBar(toolbar)

        self.history_dock = QDockWidget("History", self)
        self.history_dock.setAllowedAreas(Qt.LeftDockWidgetArea | Qt.RightDockWidgetArea)
        self.history_view = QUndoView(self.undo_stack, self.history_dock)
        self.history_view.setEmptyLabel("Nothing to undo")
        self.history_dock.setWidget(self.history_view)
        self.addDockWidget(Qt.RightDockWidgetArea, self.history_dock)
        self.history_dock.visibilityChanged.connect(self.toggle_history_action.setChecked)
        self.history_dock.hide()

    # ------------------------------------------------------------------
    def _set_mode(self, mode: GraphMode) -> None:
        self.scene.set_mode(mode)
        if mode == GraphMode.POINTER:
            self.view.setDragMode(self.view.DragMode.RubberBandDrag)
            self.pointer_mode_action.setChecked(True)
        else:
            self.view.setDragMode(self.view.DragMode.NoDrag)
            self.edge_mode_action.setChecked(True)
        self.statusBar().showMessage(f"Mode: {mode.name}")

    def add_node(self) -> None:
        suggested_text = f"Node {self._next_node_id}"
        summary, ok = QInputDialog.getText(self, "Create Node", "Summary", text=suggested_text)
        if not ok:
            return
        if not summary.strip():
            summary = suggested_text
        node_id = self._next_node_id
        doc_path = self.docs_dir / f"node_{node_id}.docx"
        position = self.view.mapToScene(self.view.viewport().rect().center())
        command = AddNodeCommand(self.scene, node_id, summary, str(doc_path), QPointF(position))
        self.undo_stack.push(command)
        self._next_node_id += 1
        self.statusBar().showMessage(f"Node {node_id} added", 3000)

    def delete_selection(self) -> None:
        if not self.scene.selectedItems():
            QMessageBox.information(self, "Delete", "No items selected.")
            return
        command = DeleteItemsCommand(self.scene, list(self.scene.selectedItems()))
        self.undo_stack.push(command)
        self.statusBar().showMessage("Selection deleted", 3000)

    def _refresh_scene_state(self) -> None:
        self.scene.update()
        self.view.viewport().update()

    def _toggle_history_dock(self, visible: bool) -> None:
        self.history_dock.setVisible(visible)

    def new_project(self) -> None:
        self.scene.clear()
        self.undo_stack.clear()
        self._project_path = None
        self._next_node_id = 1
        self.statusBar().showMessage("New project created", 3000)

    def open_project(self) -> None:
        filename, _ = QFileDialog.getOpenFileName(self, "Open Project", str(Path.cwd()), "Project Files (*.json)")
        if not filename:
            return
        try:
            max_id = ProjectIO.load_project(self.scene, filename)
        except Exception as exc:  # pragma: no cover - UI feedback
            QMessageBox.critical(self, "Error", f"Failed to load project:\n{exc}")
            return
        self.undo_stack.clear()
        self._project_path = Path(filename)
        self._next_node_id = max_id + 1
        self.statusBar().showMessage(f"Loaded project: {filename}", 3000)

    def save_project(self) -> None:
        if self._project_path is None:
            self.save_project_as()
            return
        self._perform_save(self._project_path)

    def save_project_as(self) -> None:
        filename, _ = QFileDialog.getSaveFileName(self, "Save Project", str(Path.cwd()), "Project Files (*.json)")
        if not filename:
            return
        self._project_path = Path(filename)
        self._perform_save(self._project_path)

    def _perform_save(self, path: Path) -> None:
        try:
            ProjectIO.save_project(self.scene, path)
        except Exception as exc:  # pragma: no cover - UI feedback
            QMessageBox.critical(self, "Error", f"Failed to save project:\n{exc}")
            return
        self.statusBar().showMessage(f"Project saved to {path}", 3000)
        self.setWindowTitle(f"Visual Novel Script Editor - {path.name}")

    # ------------------------------------------------------------------
    def closeEvent(self, event) -> None:  # type: ignore[override]
        # Future work: track dirty state. For now we simply close.
        super().closeEvent(event)
