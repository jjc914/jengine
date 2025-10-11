#ifndef editor_gui_PANEL_HPP
#define editor_gui_PANEL_HPP

#include <string>

namespace editor::ui {

class Panel {
public:
    Panel(const std::string& name) : _name(name), _visible(true) {}
    virtual ~Panel() = default;

    virtual void on_draw() = 0;
    virtual void on_resize(uint32_t width, uint32_t height) {}

    const std::string& name() const { return _name; }
    bool visible() const { return _visible; }
    void set_visible(bool v) { _visible = v; }

protected:
    std::string _name;
    bool _visible;
};

} // namespace editor::ui

#endif // editor_gui_PANEL_HPP