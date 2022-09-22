#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

void PlayMode::render_at(std::string txt, uint32_t x, uint32_t y) {
	hb_buffer_t* hb_buffer;
	hb_buffer = hb_buffer_create();
	hb_buffer_add_utf8(hb_buffer, txt.c_str(), -1, 0, -1);
	hb_buffer_guess_segment_properties(hb_buffer);
	hb_shape(hb_font, hb_buffer, NULL, 0);

	glm::vec2 cursor = glm::vec2(x, y);
	unsigned int len = hb_buffer_get_length(hb_buffer);
	hb_glyph_info_t* glyph_infos = hb_buffer_get_glyph_infos(hb_buffer, NULL);
	hb_glyph_position_t* glyph_positions = hb_buffer_get_glyph_positions(hb_buffer, NULL);

	for (uint32_t i = 0; i < len; i++) {
		hb_codepoint_t glyph_index = glyph_infos[i].codepoint;
		FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
		FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);
		// Somehow draw this to the screen at cursor+xoffset, cursor+yoffset
		// Potentially line wrap if cursor.x is too big
		cursor += glm::vec2(glyph_positions[i].x_advance, glyph_positions[i].y_advance);
	}
}

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	FT_Init_FreeType(&ft_library);
	FT_New_Face(ft_library, data_path("PTSerif-Italic.ttf").c_str(), 0, &ft_face);
	hb_font = hb_ft_font_create(ft_face, NULL);

	current_choice = Choice::NONE;
	current_location = Location::PRISON;
	items.clear();
	message = IN_PRISON_MESSAGE;
	left_choice = DIG_TUNNEL_CHOICE;
	right_choice = CALL_GUARD_CHOICE;
	result = DEFAULT_RESULT;
}

PlayMode::~PlayMode() {
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
	}

	return false;
}

void PlayMode::update(float elapsed) {
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
				message = DUNGEON_MESSAGE;
				left_choice = TURN_LEFT_CHOICE;
				right_choice = TURN_RIGHT_CHOICE;
				result = KNOCK_OUT_RESULT;
				items.erase(Item::ROCK);
			} else if (right_choice == TRY_LEFT_CHOICE) {
				current_location = Location::TABLE;
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
			message = AT_TABLE_MESSAGE;
			left_choice = TRY_RIGHT_CHOICE;
			right_choice = CELL_RETURN_CHOICE;
			result = LEFT_TURN_RESULT;
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::GUARDS;
			message = NEAR_GUARDS_MESSAGE;
			left_choice = CHARGE_CHOICE;
			right_choice = DISTRACTION_CHOICE;
			result = RIGHT_TURN_RESULT;
		}
		break;
	case TABLE:
		items.insert(Item::SWORD);
		items.insert(Item::SHOVEL);
		if (current_choice == Choice::LEFT) {
			current_location = Location::GUARDS;
			message = NEAR_GUARDS_MESSAGE;
			left_choice = CHARGE_CHOICE;
			right_choice = DISTRACTION_CHOICE;
			result = RIGHT_TURN_RESULT;
		} else if (current_choice == Choice::RIGHT) {
			current_location = Location::COAST;
			message = COAST_MESSAGE;
			left_choice = NO_CHOICE;
			right_choice = NO_CHOICE;
			result = LEFT_CELL_RESULT;
			items.erase(Item::SHOVEL);
		}
		break;
	case GUARDS:
		if (current_choice == Choice::LEFT) {
			if (items.find(Item::SWORD) != items.end()) {
				injured = true;
				current_location = Location::COAST;
				message = COAST_MESSAGE;
				left_choice = NO_CHOICE;
				right_choice = NO_CHOICE;
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
					message = COAST_MESSAGE;
					left_choice = NO_CHOICE;
					right_choice = NO_CHOICE;
					result = DISTRACTION_SHOVEL_RESULT;
					items.erase(Item::SHOVEL);
				} else if (items.find(Item::ROCK) != items.end()) {
					current_location = Location::COAST;
					message = COAST_MESSAGE;
					left_choice = NO_CHOICE;
					right_choice = NO_CHOICE;
					result = DISTRACTION_ROCK_RESULT;
					items.erase(Item::ROCK);
				} else {
					right_choice = CELL_RETURN_CHOICE;
					result = NO_ITEM_RESULT;
				}
			} else if (right_choice == CELL_RETURN_CHOICE) {
				current_location = Location::PRISON;
				message = IN_PRISON_MESSAGE;
				left_choice = LOOK_AROUND_CHOICE;
				right_choice = TRY_LEFT_CHOICE;
				result = RIGHT_CELL_RESULT;
			}
		}
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

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.1f;
		lines.draw_text(message,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H + 1200.0f / drawable_size.y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0xff)); 
		lines.draw_text(left_choice,
				glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H + 1000.0f / drawable_size.y, 0.0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		lines.draw_text(right_choice,
			glm::vec3(-aspect + 0.1f * H + 1000.0f / drawable_size.x, -1.0 + 0.1f * H + 1000.0f / drawable_size.y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0xff));
		lines.draw_text(result,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H + 100.0f / drawable_size.y, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0xff));
	}
	GL_ERRORS();
}
