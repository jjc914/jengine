#include "gui.hpp"

namespace gen {
	imgui::imgui() : _logger(std::cout, _LOGGER_TAG) {
		_logger.set_level(log_level::DEBUG);
		_logger.log(log_level::INFO, "created imgui");

		_elements.emplace_back(_next_id++, gui_element_type::GUI_WINDOW, "First Window", glm::vec2(0, 0), glm::vec2(0.2, 0.2), true);
	}

	void imgui::update() {
		for (gui_element element : _elements) {
			if (element.is_visible) {
				if (element.type == gui_element_type::GUI_WINDOW) {
					_create_window_vertex_buffer(element);
				} else {
					throw std::runtime_error("not implemented");
				}
			}
		}
	}

	void imgui::_create_window_vertex_buffer(gui_element window) {
		assert(window.type == gui_element_type::GUI_WINDOW);
	}
}
