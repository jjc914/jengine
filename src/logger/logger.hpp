#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>

namespace gen {
	enum log_level { DEBUG, INFO, WARNING, ERROR };

	class logger {
	public:
		logger(std::ostream& stream, std::string tag) : _stream(stream), _tag(tag) { }

		void set_level(log_level level) {
			_level = level;
		}

		template <typename ...Ts>
		void log(log_level level, Ts... args) {
			if (level >= _level) {
			 	_stream << _log_level_string(level) << " " << _tag << ": ";
				_recursive_log<Ts...>(args...);
				if (level == log_level::ERROR) {
					throw std::runtime_error("error encountered, see message above");
				}
			}
		}
	private:
		std::ostream& _stream;
		std::string _tag;
		log_level _level;

		template <typename T, typename ...Ts>
		void _recursive_log(T arg, Ts... args) {
			_stream << arg;
			_recursive_log(args...);
		}

		template <typename T>
		void _recursive_log(T arg) {
			_stream << arg << std::endl;
		}

		std::string _log_level_string(log_level level) {
			switch(level) {
			case DEBUG:
				return "DEBUG  ";
			case INFO:
				return "INFO   ";
			case WARNING:
				return "WARNING";
			case ERROR:
				return "ERROR  ";
			default:
				throw std::runtime_error("not implemented");
			}
		}
	};
}

#endif