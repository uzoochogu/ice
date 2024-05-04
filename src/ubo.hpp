#ifndef UBO_HPP
#define UBO_HPP

#include "game_objects.hpp"

namespace ice {
	struct UBO {
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 view_projection;
	};

}
#endif