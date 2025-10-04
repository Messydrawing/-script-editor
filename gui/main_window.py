"""Main window hosting the screenplay graph editor."""
from __future__ import annotations

from pathlib import Path
from typing import Optional

from PySide6.QtCore import QPointF, QSettings, Qt, QTimer
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

from gui.auto_save_dialog import AutoSaveSettingsDialog
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
        self._is_dirty = False
        self._has_autosave = False
        self._last_autosave_index = 0

        self.undo_stack = QUndoStack(self)
        self.undo_stack.cleanChanged.connect(self._on_clean_changed)
        self.undo_stack.indexChanged.connect(self._on_undo_stack_index_changed)

        self.scene = GraphScene(self)
        self.scene.set_undo_stack(self.undo_stack)
        self.view = GraphView(self.scene, self)
        self.setCentralWidget(self.view)

        self.undo_stack.indexChanged.connect(self._refresh_scene_state)

        self.settings = QSettings("OpenAI", "VisualNovelScriptEditor")
        self._auto_save_enabled = bool(
            self.settings.value("autosave/enabled", True, type=bool)
        )
        self._auto_save_interval_minutes = int(
            self.settings.value("autosave/interval_minutes", 2, type=int)
        )
        if self._auto_save_interval_minutes <= 0:
            self._auto_save_interval_minutes = 2

        self._auto_save_timer = QTimer(self)
        self._auto_save_timer.setSingleShot(False)
        self._auto_save_timer.timeout.connect(self._perform_autosave_if_needed)
        self._apply_auto_save_settings()

        self._create_actions()
        self._create_menus()
        self._create_toolbars()
        self.statusBar().showMessage("Ready")
        self._update_window_title()

        self._check_startup_backup()

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

        self.preferences_action = QAction("Auto Save Settings", self)
        self.preferences_action.triggered.connect(self._open_auto_save_settings)

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

        settings_menu = self.menuBar().addMenu("&Settings")
        settings_menu.addAction(self.preferences_action)

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
        self._remove_autosave_for_path(self._project_path)
        self.scene.clear()
        self.undo_stack.clear()
        self._project_path = None
        self._next_node_id = 1
        self.undo_stack.setClean()
        self._last_autosave_index = self.undo_stack.index()
        self._has_autosave = False
        self._record_last_project_path(None)
        self._set_dirty(False)
        self._start_auto_save_timer()
        self._update_window_title()
        self.statusBar().showMessage("New project created", 3000)

    def open_project(self) -> None:
        filename, _ = QFileDialog.getOpenFileName(self, "Open Project", str(Path.cwd()), "Project Files (*.json)")
        if not filename:
            return
        path = Path(filename)

        decision = self._prompt_restore_backup(path)
        if decision is None:
            return
        if decision:
            return
        try:
            max_id = ProjectIO.load_project(self.scene, path)
        except Exception as exc:  # pragma: no cover - UI feedback
            QMessageBox.critical(self, "Error", f"Failed to load project:\n{exc}")
            return
        self._complete_project_load(path, max_id, mark_clean=True)
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
        self.undo_stack.setClean()
        self._set_dirty(False)
        self._record_last_project_path(path)
        self._remove_autosave_for_path(path)
        self._last_autosave_index = self.undo_stack.index()
        self.statusBar().showMessage(f"Project saved to {path}", 3000)
        self._update_window_title()

    # ------------------------------------------------------------------
    def _complete_project_load(self, path: Path, max_id: int, *, mark_clean: bool) -> None:
        self.scene.update()
        self.undo_stack.clear()
        self.undo_stack.setClean()
        self._project_path = path
        self._next_node_id = max_id + 1
        self._last_autosave_index = self.undo_stack.index()
        self._record_last_project_path(path)
        self._has_autosave = not mark_clean
        self._set_dirty(not mark_clean)
        self._start_auto_save_timer()
        self._update_window_title()

    # ------------------------------------------------------------------
    def _apply_auto_save_settings(self) -> None:
        interval_ms = max(1, self._auto_save_interval_minutes) * 60 * 1000
        self._auto_save_timer.setInterval(interval_ms)
        if self._auto_save_timer.isActive():
            self._start_auto_save_timer()

    # ------------------------------------------------------------------
    def _start_auto_save_timer(self) -> None:
        if self._auto_save_enabled:
            self._auto_save_timer.start()
        else:
            self._auto_save_timer.stop()

    # ------------------------------------------------------------------
    def _perform_autosave_if_needed(self) -> None:
        if not self._auto_save_enabled or not self._is_dirty:
            return
        autosave_path = self._autosave_path_for(self._project_path)
        if autosave_path is None:
            return
        current_index = self.undo_stack.index()
        if self._has_autosave and current_index == self._last_autosave_index:
            return
        try:
            autosave_path.parent.mkdir(parents=True, exist_ok=True)
            ProjectIO.save_project(self.scene, autosave_path)
        except Exception as exc:  # pragma: no cover - UI feedback
            self.statusBar().showMessage(f"Auto-save failed: {exc}", 5000)
            return
        self._has_autosave = True
        self._last_autosave_index = current_index
        self.statusBar().showMessage(f"Backup saved to {autosave_path}", 3000)

    # ------------------------------------------------------------------
    def _autosave_path_for(self, path: Optional[Path]) -> Optional[Path]:
        if path is None:
            return None
        return path.with_suffix(path.suffix + ".autosave")

    # ------------------------------------------------------------------
    def _remove_autosave_for_path(self, path: Optional[Path]) -> None:
        if path is None:
            return
        autosave_path = self._autosave_path_for(path)
        if autosave_path is not None and autosave_path.exists():
            try:
                autosave_path.unlink()
            except OSError:
                return
        if self._project_path == path:
            self._has_autosave = False

    # ------------------------------------------------------------------
    def _on_clean_changed(self, clean: bool) -> None:
        self._set_dirty(not clean)

    # ------------------------------------------------------------------
    def _on_undo_stack_index_changed(self, _index: int) -> None:
        self._set_dirty(not self.undo_stack.isClean())

    # ------------------------------------------------------------------
    def _set_dirty(self, dirty: bool) -> None:
        if self._is_dirty == dirty:
            return
        self._is_dirty = dirty
        if not dirty:
            self._last_autosave_index = self.undo_stack.index()
        self._update_window_title()

    # ------------------------------------------------------------------
    def _update_window_title(self) -> None:
        base = "Visual Novel Script Editor"
        if self._project_path is None:
            name = "Untitled"
        else:
            name = self._project_path.name
        if self._is_dirty:
            name = f"*{name}"
        self.setWindowTitle(f"{base} - {name}")

    # ------------------------------------------------------------------
    def _prompt_restore_backup(self, path: Path) -> Optional[bool]:
        autosave_path = self._autosave_path_for(path)
        if autosave_path is None or not autosave_path.exists():
            return False
        message = (
            "A backup file was found.\n"
            "Would you like to restore the automatic backup or discard it?"
        )
        box = QMessageBox(self)
        box.setIcon(QMessageBox.Icon.Warning)
        box.setWindowTitle("Restore Backup")
        box.setText(message)
        restore_button = box.addButton("Restore", QMessageBox.ButtonRole.AcceptRole)
        discard_button = box.addButton("Discard", QMessageBox.ButtonRole.DestructiveRole)
        box.addButton(QMessageBox.StandardButton.Cancel)
        box.setDefaultButton(QMessageBox.StandardButton.Cancel)
        box.exec()

        clicked = box.clickedButton()
        if clicked is restore_button:
            try:
                max_id = ProjectIO.load_project(self.scene, autosave_path)
            except Exception as exc:  # pragma: no cover - UI feedback
                QMessageBox.critical(self, "Error", f"Failed to restore backup:\n{exc}")
                return None
            self._complete_project_load(path, max_id, mark_clean=False)
            self.statusBar().showMessage(f"Restored backup from {autosave_path}", 5000)
            return True
        if clicked is discard_button:
            self._remove_autosave_for_path(path)
            return False
        return None

    # ------------------------------------------------------------------
    def _check_startup_backup(self) -> None:
        last_path = self.settings.value("autosave/last_project", "", type=str)
        if not last_path:
            return
        path = Path(last_path)
        decision = self._prompt_restore_backup(path)
        if decision:
            return
        if decision is False and not path.exists():
            self._remove_autosave_for_path(path)

    # ------------------------------------------------------------------
    def _record_last_project_path(self, path: Optional[Path]) -> None:
        if path is None:
            self.settings.remove("autosave/last_project")
        else:
            self.settings.setValue("autosave/last_project", str(path))

    # ------------------------------------------------------------------
    def _open_auto_save_settings(self) -> None:
        dialog = AutoSaveSettingsDialog(
            enabled=self._auto_save_enabled,
            interval_minutes=self._auto_save_interval_minutes,
            parent=self,
        )
        if dialog.exec() != AutoSaveSettingsDialog.DialogCode.Accepted:
            return
        enabled, minutes = dialog.values()
        self._auto_save_enabled = enabled
        self._auto_save_interval_minutes = max(1, minutes)
        self.settings.setValue("autosave/enabled", enabled)
        self.settings.setValue("autosave/interval_minutes", self._auto_save_interval_minutes)
        self._apply_auto_save_settings()
        self._start_auto_save_timer()

    # ------------------------------------------------------------------
    def closeEvent(self, event) -> None:  # type: ignore[override]
        super().closeEvent(event)
