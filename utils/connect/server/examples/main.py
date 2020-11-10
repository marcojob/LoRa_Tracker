import sys
from PyQt5.QtWidgets import QApplication, QVBoxLayout, QWidget, QPushButton
from pyqtlet import L, MapWidget
from random import uniform


class MapWindow(QWidget):
    def __init__(self):
        # Setting up the widgets and layout
        super().__init__()
        self.mapWidget = MapWidget()
        self.button_refresh = QPushButton('Refresh')

        self.layout = QVBoxLayout()
        self.layout.addWidget(self.mapWidget)
        self.layout.addWidget(self.button_refresh)
        self.button_refresh.clicked.connect(self.refresh_data)
        self.setLayout(self.layout)

        # Working with the maps with pyqtlet
        self.map = L.map(self.mapWidget)
        self.map.setView([47.377621, 8.541238], 12.25)
        L.tileLayer('http://{s}.tile.osm.org/{z}/{x}/{y}.png').addTo(self.map)
        self.marker = L.marker([47.404749, 8.554875])
        self.marker.bindPopup('Maps are a treasure.')
        self.map.addLayer(self.marker)
        self.show()

    def refresh_data(self):
        self.marker = L.circleMarker([47.404749-uniform(-1,1)/50,
                                8.554875-uniform(-1,1)/50], {'radius': 5, 'color': '#C62828', 'className': 'L'})
        self.map.addLayer(self.marker)
        self.show()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    widget = MapWindow()
    sys.exit(app.exec_())
