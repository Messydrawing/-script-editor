"""Dialog allowing users to configure auto-save behavior."""
from __future__ import annotations

from PySide6.QtWidgets import (
    QCheckBox,
    QDialog,
    QDialogButtonBox,
    QFormLayout,
    QSpinBox,
)


class AutoSaveSettingsDialog(QDialog):
    """Simple settings dialog for configuring automatic backups."""

    def __init__(self, enabled: bool, interval_minutes: int, parent=None) -> None:
        super().__init__(parent)
        self.setWindowTitle("Auto Save Settings")

        layout = QFormLayout(self)

        self.enabled_checkbox = QCheckBox("Enable automatic backups", self)
        self.enabled_checkbox.setChecked(enabled)
        layout.addRow(self.enabled_checkbox)

        self.interval_spinbox = QSpinBox(self)
        self.interval_spinbox.setRange(1, 120)
        self.interval_spinbox.setSuffix(" minutes")
        self.interval_spinbox.setValue(max(1, interval_minutes))
        layout.addRow("Interval", self.interval_spinbox)

        buttons = QDialogButtonBox(
            QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel,
            parent=self,
        )
        buttons.accepted.connect(self.accept)
        buttons.rejected.connect(self.reject)
        layout.addRow(buttons)

    # ------------------------------------------------------------------
    def values(self) -> tuple[bool, int]:
        """Return the state of the form elements."""

        return self.enabled_checkbox.isChecked(), int(self.interval_spinbox.value())
