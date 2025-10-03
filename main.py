"""Program entry point for the visual novel script editor."""
import sys
from PySide6.QtWidgets import QApplication

from gui.main_window import MainWindow


def main() -> int:
    """Create the application and launch the main window."""
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
