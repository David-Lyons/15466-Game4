#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"
#include "ColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("sets.pnct"));
	meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > sets(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("sets.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

void PlayMode::render_at(std::string txt, float x, float y, glm::uvec2 const& drawable_size) {
	// Harfbuzz code based on https://github.com/harfbuzz/harfbuzz-tutorial/blob/master/hello-harfbuzz-freetype.c
	hb_buffer_clear_contents(hb_buffer);
	hb_buffer_add_utf8(hb_buffer, txt.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);
	hb_shape(hb_font, hb_buffer, NULL, 0);

	glm::vec2 cursor = glm::vec2(x, y);
	uint32_t len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t* glyph_infos = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t* glyph_positions = hb_buffer_get_glyph_positions(hb_buffer, NULL);

	// GL code based on https://learnopengl.com/In-Practice/Text-Rendering
	GLuint texture; 
	unsigned int VAO, VBO;
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glUseProgram(color_texture_program->program);
	glUniform3f(color_texture_program->textColor_vec3, 0.2f, 0.8f, 0.6f);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(VAO);
	glm::mat4 projection = glm::ortho(0.0f, (float)drawable_size.x, 0.0f, (float)drawable_size.y);
	glUniformMatrix4fv(color_texture_program->projection_mat4, 1, GL_FALSE, &projection[0][0]);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	for (uint32_t i = 0; i < len; i++) {
		// FT code based on https://freetype.org/freetype2/docs/tutorial/step1.html
		hb_codepoint_t glyph_index = glyph_infos[i].codepoint;
		FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
		FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
		
		// From GL tutorial
		try {
			texture = texture_map.at(glyph_index);
			glBindTexture(GL_TEXTURE_2D, texture);
		} catch (std::out_of_range) {
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ft_face->glyph->bitmap.width, ft_face->glyph->bitmap.rows,
				0, GL_RED, GL_UNSIGNED_BYTE, ft_face->glyph->bitmap.buffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		float xpos = cursor.x + ft_face->glyph->bitmap_left;
		float ypos = cursor.y + ft_face->glyph->bitmap_top - ft_face->glyph->bitmap.rows;
		float w = (float)ft_face->glyph->bitmap.width;
		float h = (float)ft_face->glyph->bitmap.rows;
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },

			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		cursor += glm::vec2(glyph_positions[i].x_advance >> 6, glyph_positions[i].y_advance >> 6);
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

PlayMode::PlayMode() : scene(*sets) {
	//get pointer to camera for convenience:
	int camera_number = 0;
	for (auto& cmra : scene.cameras) {
		if (camera_number == 0) {
			prison_camera = &cmra;
		}
		if (camera_number == 1) {
			coast_camera = &cmra;
		}
		if (camera_number == 2) {
			table_camera = &cmra;
		}
		if (camera_number == 3) {
			dungeon_camera = &cmra;
		}
		if (camera_number == 4) {
			guard_camera = &cmra;
		}
		if (camera_number == 5) {
			raft_camera = &cmra;
		}
		if (camera_number == 6) {
			deeper_forest_camera = &cmra;
		}
		if (camera_number == 7) {
			crate_camera = &cmra;
		}
		if (camera_number == 8) {
			ship_camera = &cmra;
		}
		if (camera_number == 9) {
			oar_camera = &cmra;
		}
		if (camera_number == 10) {
			coastguard_camera = &cmra;
		}
		if (camera_number == 11) {
			forest_camera = &cmra;
		}
		camera_number++;
	}
	camera = prison_camera;

	// From Harfbuzz tutorial
	FT_Init_FreeType(&ft_library);
	FT_New_Face(ft_library, data_path("PTSerif-Italic.ttf").c_str(), 0, &ft_face);
	// Help from Sarah Pethani debugging my font size (originally I didn't multiply by 64)
	FT_Set_Char_Size(ft_face, 36 * 64, 36 * 64, 0, 0);
	hb_font = hb_ft_font_create(ft_face, NULL);
	hb_buffer = hb_buffer_create();

	current_choice = Choice::NONE;
	current_location = Location::PRISON;
	items.clear();
	message = IN_PRISON_MESSAGE;
	left_choice = DIG_TUNNEL_CHOICE;
	right_choice = CALL_GUARD_CHOICE;
	result = DEFAULT_RESULT;
}

PlayMode::~PlayMode() {
	FT_Done_FreeType(ft_library);
	FT_Done_Face(ft_face);
	hb_buffer_destroy(hb_buffer);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (current_choice != Choice::NONE) {
		return false;
	}

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			current_choice = Choice::LEFT;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			current_choice = Choice::RIGHT;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_r) {
			current_location = Location::PRISON;
			camera = prison_camera;
			message = IN_PRISON_MESSAGE;
			left_choice = DIG_TUNNEL_CHOICE;
			right_choice = CALL_GUARD_CHOICE;
			result = DEFAULT_RESULT;
			past_guard = false;
			buddy = false;
			injured = false;
			time_to_crate = 0.0f;
			elapsed_time = 0.0f;
			items.clear();
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	elapsed_time += elapsed;
	if (current_choice == Choice::NONE) {
		return;
	}
	switch (current_location) {
	case PRISON:
		if (current_choice == Choice::LEFT) {
			if (left_choice == DIG_TUNNEL_CHOICE) {
				left_choice = LOOK_AROUND_CHOICE;
				result = NO_ITEM_RESULT;
			} else if (left_choice == LOOK_AROUND_CHOICE) {
				if (past_guard) {
					left_choice = TRY_RIGHT_CHOICE;
				} else {
					left_choice = GIVE_UP_CHOICE;
				}
				items.insert(Item::ROCK);
				result = LOOK_AROUND_RESULT;
			} else if (left_choice == ASK_NICELY_CHOICE) {
				message = NO_CHOICE;
				left_choice = NO_CHOICE;
				right_choice = NO_CHOICE;
				result = GUARD_LEAVES_RESULT;
			} else if (left_choice == TRY_RIGHT_CHOICE) {
				current_location = Location::GUARDS;
				camera = guard_camera;
				message = NEAR_GUARDS_MESSAGE;
				left_choice = CHARGE_CHOICE;
				right_choice = DISTRACTION_CHOICE;
				result = RIGHT_TURN_RESULT;
			} else if (left_choice == GIVE_UP_CHOICE) {
				message = NO_CHOICE;
				left_choice = NO_CHOICE;
				right_choice = NO_CHOICE;
				result = GIVE_UP_RESULT;
			}
		} else if (current_choice == Choice::RIGHT) {
			if (right_choice == CALL_GUARD_CHOICE) {
				left_choice = ASK_NICELY_CHOICE;
				if (items.find(Item::ROCK) != items.end()) {
					right_choice = USE_ROCK_CHOICE;
				} else {
					right_choice = ATTACK_GUARD_CHOICE;
				}
				result = CALL_GUARD_RESULT;
			} else if (right_choice == ATTACK_GUARD_CHOICE || right_choice == USE_ROCK_CHOICE) {
				past_guard = true;
				current_location = Location::DUNGEON;
				camera = dungeon_camera;
				message = DUNGEON_MESSAGE;
				left_choice = TURN_LEFT_CHOICE;
				right_choice = TURN_RIGHT_CHOICE;
				result = KNOCK_OUT_RESULT;
			} else if (right_choice == TRY_LEFT_CHOICE) {
				current_location = Location::TABLE;
				camera = table_camera;
				message = AT_TABLE_MESSAGE;
				left_choice = TRY_RIGHT_CHOICE;
				right_choice = CELL_RETURN_CHOICE;
				result = LEFT_TURN_RESULT;
			}
		}
		break;
	case DUNGEON:
		if (current_choice == Choice::LEFT) {
			current_location = Location::TABLE;
			camera = table_camera;
			message = AT_TABLE_MESSAGE;
			left_choice = TRY_RIGHT_CHOICE;
			right_choice = CELL_RETURN_CHOICE;
			result = LEFT_TURN_RESULT;
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::GUARDS;
			camera = guard_camera;
			message = NEAR_GUARDS_MESSAGE;
			left_choice = CHARGE_CHOICE;
			right_choice = DISTRACTION_CHOICE;
			result = RIGHT_TURN_RESULT;
		}
		break;
	case TABLE:
		if (items.find(Item::SWORD) == items.end()) {
			items.insert(Item::SWORD);
			items.insert(Item::SHOVEL);
		}
		if (current_choice == Choice::LEFT) {
			current_location = Location::GUARDS;
			camera = guard_camera;
			message = NEAR_GUARDS_MESSAGE;
			left_choice = CHARGE_CHOICE;
			right_choice = DISTRACTION_CHOICE;
			result = RIGHT_TURN_RESULT;
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::COAST;
			camera = coast_camera;
			message = COAST_MESSAGE;
			left_choice = TURN_LEFT_CHOICE;
			right_choice = TURN_RIGHT_CHOICE;
			result = LEFT_CELL_RESULT;
		}
		break;
	case GUARDS:
		if (current_choice == Choice::LEFT && left_choice != NO_CHOICE) {
			if (items.find(Item::SWORD) != items.end()) {
				injured = true;
				current_location = Location::COAST;
				camera = coast_camera;
				message = COAST_MESSAGE;
				left_choice = TURN_LEFT_CHOICE;
				right_choice = TURN_RIGHT_CHOICE;
				result = FIGHT_SWORD_RESULT;
			} else {
				message = NO_CHOICE;
				left_choice = NO_CHOICE;
				right_choice = NO_CHOICE;
				result = FIGHT_NO_SWORD_RESULT;
			}
		} else if (current_choice == Choice::RIGHT) {
			if (right_choice == DISTRACTION_CHOICE) {
				if (items.find(Item::SHOVEL) != items.end()) {
					current_location = Location::COAST;
					camera = coast_camera;
					message = COAST_MESSAGE;
					left_choice = TURN_LEFT_CHOICE;
					right_choice = TURN_RIGHT_CHOICE;
					result = DISTRACTION_SHOVEL_RESULT;
				} else if (items.find(Item::ROCK) != items.end()) {
					current_location = Location::COAST;
					camera = coast_camera;
					message = COAST_MESSAGE;
					left_choice = TURN_LEFT_CHOICE;
					right_choice = TURN_RIGHT_CHOICE;
					result = DISTRACTION_ROCK_RESULT;
				} else {
					right_choice = CELL_RETURN_CHOICE;
					result = NO_ITEM_RESULT;
				}
			} else if (right_choice == CELL_RETURN_CHOICE) {
				current_location = Location::PRISON;
				camera = prison_camera;
				message = IN_PRISON_MESSAGE;
				left_choice = LOOK_AROUND_CHOICE;
				right_choice = TRY_LEFT_CHOICE;
				result = RIGHT_CELL_RESULT;
			}
		}
		break;
	case COAST:
		if (current_choice == Choice::LEFT) {
			current_location = Location::CRATE;
			camera = crate_camera;
			message = CRATE_MESSAGE;
			left_choice = DEEPER_CHOICE;
			right_choice = GO_BACK_CHOICE;
			if (time_to_crate == 0.0f) {
				time_to_crate = elapsed_time;
				result = CRATE_TTC_RESULT + std::to_string((size_t)time_to_crate) + " seconds.";
			} else {
				result = CRATE_RESULT;
			}
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::FOREST;
			camera = forest_camera;
			message = FOREST_MESSAGE;
			left_choice = DEEPER_CHOICE;
			right_choice = GO_BACK_CHOICE;
			if (buddy) {
				result = FOREST_RESULT;
			} else if (items.find(Item::KEY) != items.end()) {
				buddy = true;
				result = FOREST_CREW_FREE_RESULT;
			} else {
				result = FOREST_CREW_LOCKED_RESULT;
			}
		} 
		break;
	case CRATE:
		if (items.find(Item::KEY) == items.end()) {
			items.insert(Item::KEY);
		}
		if (current_choice == Choice::LEFT) {
			current_location = Location::COASTGUARDS;
			camera = coastguard_camera;
			message = NEAR_GUARDS_MESSAGE;
			left_choice = CHARGE_CHOICE;
			right_choice = GO_BACK_CHOICE;
			result = RIGHT_TURN_RESULT;
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::COAST;
			camera = coast_camera;
			message = COAST_MESSAGE;
			left_choice = TURN_LEFT_CHOICE;
			right_choice = TURN_RIGHT_CHOICE;
			result = COAST_RETURN_RESULT;
		}
		break;
	case COASTGUARDS:
		if (current_choice == Choice::LEFT && left_choice != NO_CHOICE) {
			left_choice = NO_CHOICE;
			right_choice = NO_CHOICE;
			if (items.find(Item::SWORD) != items.end() && (buddy || !injured)) {
				current_location = Location::SHIP;
				camera = ship_camera;
				message = SHIP_MESSAGE;
				result = SHIP_RESULT;
			} else {
				result = FIGHT_NO_SWORD_RESULT;
			}
		} else if (current_choice == Choice::RIGHT && right_choice != NO_CHOICE) {
			current_location = Location::CRATE;
			camera = crate_camera;
			message = CRATE_MESSAGE;
			left_choice = DEEPER_CHOICE;
			right_choice = GO_BACK_CHOICE;
			if (time_to_crate == 0.0f) {
				time_to_crate = elapsed_time;
				result = CRATE_TTC_RESULT + std::to_string((size_t)time_to_crate) + " seconds.";
			} else {
				result = CRATE_RESULT;
			}
		}
		break;
	case FOREST:
		if (current_choice == Choice::LEFT) {
			current_location = Location::DEEPWOODS;
			camera = deeper_forest_camera;
			message = DEEPWOODS_MESSAGE;
			left_choice = TURN_LEFT_CHOICE;
			right_choice = TURN_RIGHT_CHOICE;
			result = DEEPER_RESULT;
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::COAST;
			camera = coast_camera;
			message = COAST_MESSAGE;
			left_choice = TURN_LEFT_CHOICE;
			right_choice = TURN_RIGHT_CHOICE;
			result = COAST_RETURN_RESULT;
		}
		break;
	case DEEPWOODS:
		if (current_choice == Choice::LEFT) {
			current_location = Location::OAR_LOC;
			camera = oar_camera;
			message = OAR_MESSAGE;
			left_choice = GIVE_UP_CHOICE;
			right_choice = GO_BACK_CHOICE;
			if (items.find(Item::OAR) == items.end()) {
				items.insert(Item::OAR);
				result = FOUND_OAR_RESULT;
			} else {
				result = NOTHING_ELSE_RESULT;
			}
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::RAFT;
			camera = raft_camera;
			if (items.find(Item::OAR) != items.end()) {
				message = RAFT_WIN;
				left_choice = NO_CHOICE;
				right_choice = NO_CHOICE;
				result = RAFT_OAR_RESULT;
			} else {
				message = RAFT_MESSAGE;
				left_choice = GIVE_UP_CHOICE;
				right_choice = GO_BACK_CHOICE;
				result = RAFT_NO_OAR_RESULT;
			}
		}
		break;
	case OAR_LOC:
		if (current_choice == Choice::LEFT && left_choice != NO_CHOICE) {
			left_choice = NO_CHOICE;
			right_choice = NO_CHOICE;
			result = GIVE_UP_RESULT;
		} else if (current_choice == Choice::RIGHT && right_choice != NO_CHOICE) {
			current_location = Location::DEEPWOODS;
			camera = deeper_forest_camera;
			message = DEEPWOODS_MESSAGE;
			left_choice = TURN_LEFT_CHOICE;
			right_choice = TURN_RIGHT_CHOICE;
			result = DEEP_RETURN_RESULT;
		}
		break;
	case RAFT:
		if (current_choice == Choice::LEFT && left_choice != NO_CHOICE) {
			left_choice = NO_CHOICE;
			right_choice = NO_CHOICE;
			result = GIVE_UP_RESULT;
		} else if (current_choice == Choice::RIGHT && right_choice != NO_CHOICE) {
			current_location = Location::DEEPWOODS;
			camera = deeper_forest_camera;
			message = DEEPWOODS_MESSAGE;
			left_choice = TURN_LEFT_CHOICE;
			right_choice = TURN_RIGHT_CHOICE;
			result = DEEP_RETURN_RESULT;
		}
		break;
	case SHIP:
		break;
	}
	current_choice = Choice::NONE;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	glDisable(GL_DEPTH_TEST);
	render_at(message, drawable_size.x / 10.0f, drawable_size.y * 5.0f / 6.0f, drawable_size);
	render_at(left_choice, drawable_size.x / 10.0f, drawable_size.y * 4.0f / 6.0f, drawable_size);
	render_at(right_choice, drawable_size.x / 2.0f, drawable_size.y * 4.0f / 6.0f, drawable_size);
	render_at(result, drawable_size.x / 10.0f, drawable_size.x / 8.0f, drawable_size);
	GL_ERRORS();
}
