# Boxxy plugin (Python version)
# Mirrors contrib/boxxy_plugin/boxxy_plugin.rb

import pya


# Module-level default layer storage (session-scoped)
_default_cv = None
_default_li = None


def set_default_layer(cv: int, li: int) -> None:
    global _default_cv, _default_li
    _default_cv = cv
    _default_li = li


def clear_default_layer() -> None:
    global _default_cv, _default_li
    _default_cv = None
    _default_li = None


def get_default_layer():
    return _default_cv, _default_li


def has_default_layer() -> bool:
    return _default_cv is not None and _default_li is not None


def _editor_mode() -> bool:
    try:
        app = pya.Application.instance()
        if app and hasattr(app, "is_editable") and app.is_editable():
            return True
        mw = app and app.main_window()
        view = mw and mw.current_view()
        return bool(view and hasattr(view, "is_editable") and view.is_editable())
    except Exception:
        return False


def _viewer_warning_suppressed() -> bool:
    try:
        app = pya.Application.instance()
        v = app and app.get_config("boxxy_plugin.viewer_warning_suppressed")
        return str(v) == "1"
    except Exception:
        return False


def _suppress_viewer_warning() -> None:
    try:
        app = pya.Application.instance()
        if app:
            app.set_config("boxxy_plugin.viewer_warning_suppressed", "1")
            if hasattr(app, "commit_config"):
                app.commit_config()
    except Exception:
        pass


class BoxxyFactory(pya.PluginFactory):
    def __init__(self):
        super(BoxxyFactory, self).__init__()

        if _editor_mode():
            # Create Edit > Boxxy submenu and actions (before register)
            self.add_submenu("boxxy_menu", "edit_menu.end", "Boxxy")
            self.add_menu_entry("boxxy_set_default", "boxxy_menu_set", "edit_menu.boxxy_menu.end",
                                "Set Boxxy Default Layer From Current")
            self.add_menu_entry("boxxy_clear_default", "boxxy_menu_clear", "edit_menu.boxxy_menu.end",
                                "Clear Boxxy Default Layer")

            self.register(50010, "boxxy:edit_mode", "Boxxy", ":box_24px.png")
        else:
            if not _viewer_warning_suppressed():
                app = pya.Application.instance()
                mw = app and app.main_window()
                text = (
                    "Boxxy is available in Editor mode only.\n\n"
                    "Start KLayout in Editor mode (e.g. with -e) to use this plugin."
                )
                mb = pya.QMessageBox(pya.QMessageBox.Warning, "Boxxy", text, pya.QMessageBox.Ok, mw)
                cb = pya.QCheckBox("Don't show this again")
                mb.setCheckBox(cb)
                mb.exec()
                if cb.isChecked():
                    _suppress_viewer_warning()

    def create_plugin(self, manager, dispatcher, view):
        return BoxxyTool(view)


class BoxxyTool(pya.Plugin):
    def __init__(self, view):
        super(BoxxyTool, self).__init__()
        self._view = view
        self._editing = False
        self._p1 = None
        self._marker = None
        self._target_cv_idx = None
        self._target_layer_idx = None

    def activated(self):
        self._cancel_edit()

    def deactivated(self):
        self._cancel_edit()

    def mouse_click_event(self, p, buttons, prio):
        if not prio:
            return False

        try:
            if not self._editing:
                self._ensure_current_layer()
                self._p1 = p
                self._editing = True
                self._ensure_marker()
                self._update_marker(p)
                self.set_cursor(pya.Cursor.Cross)
                return True
            else:
                self._insert_box(self._p1, p)
                self._cancel_edit()
                return True
        except Exception as e:
            try:
                pya.MessageBox.warning("Boxxy", str(e), pya.MessageBox.Ok)
            except Exception:
                pass
            self._cancel_edit()
            return True

    def mouse_moved_event(self, p, buttons, prio):
        if not prio:
            return False
        if self._editing:
            self._ensure_marker()
            self._update_marker(p)
            self.set_cursor(pya.Cursor.Cross)
            return True
        return False

    def key_event(self, key, buttons):
        # Qt::Key_Escape == 0x01000000
        if self._editing and key == 0x01000000:
            self._cancel_edit()
            return True
        return False

    def menu_activated(self, symbol):
        if symbol == "boxxy_set_default":
            it = self._view.current_layer()
            if it.is_null():
                pya.MessageBox.warning("Boxxy", "No current layer is selected.", pya.MessageBox.Ok)
                return True
            n = it.current()
            cv = n.cellview
            li = n.layer_index
            if cv < 0 or li < 0:
                pya.MessageBox.warning("Boxxy", "Current layer is not a valid drawing layer.", pya.MessageBox.Ok)
                return True
            set_default_layer(cv, li)
            pya.MessageBox.info("Boxxy", f"Default layer set to CV={cv}, layer={li}.", pya.MessageBox.Ok)
            return True
        elif symbol == "boxxy_clear_default":
            clear_default_layer()
            pya.MessageBox.info("Boxxy", "Default layer cleared.", pya.MessageBox.Ok)
            return True
        return False

    # Internal helpers
    def _ensure_current_layer(self):
        chosen = None

        # If there is an explicit selection, use the current layer
        try:
            sel = self._view.selected_layers()
        except Exception:
            sel = []
        if sel is None:
            sel = []
        if len(sel) > 0:
            chosen = self._view.current_layer()

        # If none chosen, try Boxxy's default layer
        if chosen is None and has_default_layer():
            cv, li = get_default_layer()
            if cv is not None and li is not None and cv >= 0 and li >= 0:
                it = self._view.begin_layers()
                while not it.at_end():
                    n = it.current()
                    if not n.has_children():
                        if n.cellview == cv and n.layer_index == li:
                            chosen = it
                            break
                    it.next()

        # Fallback: first drawing layer
        if chosen is None:
            it = self._view.begin_layers()
            while not it.at_end():
                n = it.current()
                if not n.has_children():
                    li = n.layer_index
                    cv = n.cellview
                    if cv >= 0 and li >= 0:
                        chosen = it
                        break
                it.next()

        if chosen is None:
            raise RuntimeError("Please select or define a drawing layer first")

        # Apply chosen iterator and cache indices
        self._view.current_layer = chosen
        n = chosen.current()
        self._target_cv_idx = n.cellview
        self._target_layer_idx = n.layer_index
        if self._target_cv_idx < 0 or self._target_layer_idx < 0:
            raise RuntimeError("Selected layer is not valid")

    def _ensure_marker(self):
        if self._marker is None:
            self._marker = pya.Marker(self._view)
            self._marker.dismissable = True
            self._marker.line_width = 1
            self._marker.vertex_size = 0
            self._marker.dither_pattern = -1

    def _release_marker(self):
        if self._marker is not None:
            try:
                if hasattr(self._marker, "destroy"):
                    self._marker.destroy()
                elif hasattr(self._marker, "_destroy"):
                    self._marker._destroy()
            except Exception:
                pass
        self._marker = None

    def _cancel_edit(self):
        self._editing = False
        self._p1 = None
        self._release_marker()
        try:
            self.set_cursor(pya.Cursor.None_)
        except Exception:
            pass

    def _update_marker(self, p2):
        if not (self._marker and self._p1):
            return
        box = pya.DBox(self._p1, p2)
        self._marker.set(box)

    def _insert_box(self, p1, p2):
        if self._target_cv_idx is None or self._target_layer_idx is None:
            raise RuntimeError("No target layer")

        cv = self._view.cellview(self._target_cv_idx)
        if not (cv and cv.is_valid()):
            raise RuntimeError("No active cellview")

        layout = cv.layout()
        cell = cv.cell
        if not cell:
            raise RuntimeError("No target cell")

        dct_inv = cv.context_dtrans().inverted()
        q1 = dct_inv.trans(p1)
        q2 = dct_inv.trans(p2)
        dbox = pya.DBox(q1, q2)
        ibox = dbox.to_itype(layout.dbu())
        cell.shapes(self._target_layer_idx).insert(ibox)


# Register the factory (instantiation triggers registration)
BoxxyFactory()

