
# Desc: Application main window
# Author: Andrew Howard
# Date: 14 Jun 2003

import gtk
import gtk.glade
import rtk3

# Flag set if there has been a quit request from the gui
quit = 0


class MainWin:
    """The map application window."""

    def __init__(self, gladepath):
        """Initialize the main window based on a Glade description."""

        gladefile = gladepath + '/playermap.glade'
        self.glade = gtk.glade.XML(gladefile)

        self.window = self.glade.get_widget('main_window')
        self.window.connect('destroy', self.on_quit)

        self.quit = 0

        # Default to 1, which is an anti-aliased canvas.  Call with 0 for 
        # X11 canvas.  Should probably expose this somehow.
        self.canvas = rtk3.Canvas(1)
        self.canvas.addto(self.glade.get_widget('globalmap'))

        #self.canvas_scale = 0.05
        self.canvas_scale = 0.06
        self.canvas.set_scale(self.canvas_scale)
        self.canvas.set_extent((-100, -100), (100, 100))

        self.root_fig = rtk3.Fig(self.canvas.root())
        #self.root_fig.bgcolor((128, 128, 128, 255))
        self.root_fig.bgcolor((255, 255, 255, 255))
        self.root_fig.rectangle((0, 0, 0), (200, 200))
        self.root_fig.connect(self.on_root_event, None)

        self.zoom_in = self.glade.get_widget('zoom_in')
        self.zoom_in.connect('clicked', self.on_zoom)
        self.zoom_out = self.glade.get_widget('zoom_out')
        self.zoom_out.connect('clicked', self.on_zoom)

        self.rotate = 0
        self.rotate_left_button = self.glade.get_widget('rotate_left')
        self.rotate_left_button.connect('clicked', self.on_press)
        self.rotate_right_button = self.glade.get_widget('rotate_right')
        self.rotate_right_button.connect('clicked', self.on_press)

        self.pause = 0
        self.pause_button = self.glade.get_widget('pause')
        self.pause_button.connect('clicked', self.on_toggle)

        self.infer_links = 0
        self.infer_links_button = self.glade.get_widget('infer_links')
        self.infer_links_button.connect('clicked', self.on_press)

        self.fit_strong = 0
        self.fit_strong_button = self.glade.get_widget('fit_strong')
        self.fit_strong_button.connect('clicked', self.on_toggle)

        self.make_grid = 0
        self.make_grid_button = self.glade.get_widget('make_grid')
        self.make_grid_button.connect('clicked', self.on_press)

        # Set default button states
        #self.pause_button.clicked()
        return


    def show(self):

        self.window.show_all()
        return
    

    def on_quit(self, *args):
        """Handle quit events."""

        global quit
        quit = 1
        return


    def on_zoom(self, button):
        """Zoom in or out."""

        if button == self.zoom_in:
            self.canvas_scale /= 1.5
            self.canvas.set_scale(self.canvas_scale)

        elif button == self.zoom_out:
            self.canvas_scale *= 1.5
            self.canvas.set_scale(self.canvas_scale)

        return


    def on_press(self, button):
        """Generic handler for push buttons."""

        if button == self.rotate_left_button:
            self.rotate += 1
        if button == self.rotate_right_button:
            self.rotate -= 1
        if button == self.infer_links_button:
            self.infer_links = 1
        if button == self.make_grid_button:
            self.make_grid = 1
        return


    def on_toggle(self, button):
        """Generic handler for toggle buttons."""

        if button == self.pause_button:
            self.pause = not self.pause
        if button == self.fit_strong_button:
            self.fit_strong = not self.fit_strong
        return


    def on_root_event(self, fig, event, pos, dummy):

        return


def do_yield():
    """Process gui events"""
    
    while gtk.events_pending():
        gtk.main_iteration_do()

    return
