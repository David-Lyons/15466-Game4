#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>
#include <hb.h>
#include <hb-ft.h>
#include <freetype/freetype.h>

#include <vector>
#include <unordered_map>
#include <set>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void render_at(std::string txt, float x, float y, glm::uvec2 const& drawable_size);

	//----- game state -----

	// Scene/cameras
	Scene scene;
	Scene::Camera* prison_camera = nullptr;
	Scene::Camera* coast_camera = nullptr;
	Scene::Camera* table_camera = nullptr;
	Scene::Camera* dungeon_camera = nullptr;
	Scene::Camera* guard_camera = nullptr;
	Scene::Camera* crate_camera = nullptr;
	Scene::Camera* forest_camera = nullptr;
	Scene::Camera* deeper_forest_camera = nullptr;
	Scene::Camera* coastguard_camera = nullptr;
	Scene::Camera* oar_camera = nullptr;
	Scene::Camera* raft_camera = nullptr;
	Scene::Camera* ship_camera = nullptr;
	Scene::Camera* camera = nullptr;

	// Text shaping
	FT_Library ft_library;
	FT_Face ft_face;
	hb_font_t* hb_font;
	hb_buffer_t* hb_buffer;
	std::unordered_map<hb_codepoint_t, GLuint> texture_map;

	// Game state
	enum Location {
		PRISON,
		DUNGEON,
		TABLE,
		GUARDS,
		COAST,
		COASTGUARDS,
		CRATE,
		SHIP,
		FOREST,
		DEEPWOODS,
		OAR_LOC,
		RAFT
	} current_location;

	enum Choice {
		LEFT,
		RIGHT,
		NONE
	} current_choice;

	enum Item {
		SWORD,
		ROCK,
		SHOVEL,
		OAR,
		KEY
	};

	std::set<Item> items;
	
	std::string message;
	std::string result;
	std::string left_choice;
	std::string right_choice;

	float time_to_crate = 0.0f;
	float elapsed_time = 0.0f;

	bool past_guard = false;
	bool injured = false;
	bool buddy = false;

	// Location messages
	std::string IN_PRISON_MESSAGE = "You are in the prison. What do you do to escape?";
	std::string AT_TABLE_MESSAGE = "You are at the table. Where do you go?";
	std::string NEAR_GUARDS_MESSAGE = "You are near the guards. What do you do?";
	std::string DUNGEON_MESSAGE = "You are in the dungeon. Where do you go?";
	std::string COAST_MESSAGE = "You are on the coast. What do you do?";
	std::string CRATE_MESSAGE = "You are by the crate. Where to next?";
	std::string FOREST_MESSAGE = "You are in the forest. What do you do?";
	std::string DEEPWOODS_MESSAGE = "You are deep in the forest. Where do you go?";
	std::string OAR_MESSAGE = "You are by an oar. Where to next?";
	std::string RAFT_MESSAGE = "You are by the raft but have nothing to sail with. What do you do?";
	std::string RAFT_WIN = "You are by the raft and have an oar. Congratulations!";
	std::string SHIP_MESSAGE = "You have found a ship to escape on. Congratulations!";

	// Choices
	std::string GIVE_UP_CHOICE = "Give up.";
	std::string GO_BACK_CHOICE = "Go back.";
	std::string NO_CHOICE = "";
	// Left choices
	std::string DIG_TUNNEL_CHOICE = "Dig a tunnel.";
	std::string ASK_NICELY_CHOICE = "Ask him nicely to let you go.";
	std::string LOOK_AROUND_CHOICE = "Look around.";
	std::string TURN_LEFT_CHOICE = "Turn left.";
	std::string TRY_RIGHT_CHOICE = "Go right this time.";
	std::string CHARGE_CHOICE = "Charge!";
	std::string DEEPER_CHOICE = "Keep going deeper.";

	// Right choices
	std::string CALL_GUARD_CHOICE = "Call the guard.";
	std::string ATTACK_GUARD_CHOICE = "Attack him!";
	std::string USE_ROCK_CHOICE = "Use the rock.";
	std::string TURN_RIGHT_CHOICE = "Turn right.";
	std::string CELL_RETURN_CHOICE = "Back to the cell.";
	std::string DISTRACTION_CHOICE = "Cause a distraction.";
	std::string TRY_LEFT_CHOICE = "Go left this time.";

	// Results
	std::string GIVE_UP_RESULT = "It's a tough game for sure! Game over.";
	std::string DEFAULT_RESULT = "Good luck!";
	// Part 1
	std::string NO_ITEM_RESULT = "With what? Nice try.";
	std::string LOOK_AROUND_RESULT = "You see a rock and pick it up.";
	std::string CALL_GUARD_RESULT = "The guard approaches.";
	std::string GUARD_LEAVES_RESULT = "The guard leaves and won't come back. Game over.";
	std::string KNOCK_OUT_RESULT = "The guard is knocked out. You take and use the key.";
	std::string LEFT_TURN_RESULT = "You see a table. There is a sword and shovel.";
	std::string RIGHT_TURN_RESULT = "You see two guards.";
	std::string LEFT_CELL_RESULT = "You use the shovel to dig your way out of the prison.";
	std::string RIGHT_CELL_RESULT = "Guess captivity is preferable to death.";
	std::string DISTRACTION_SHOVEL_RESULT = "You throw the shovel and sneak by the guards.";
	std::string DISTRACTION_ROCK_RESULT = "You throw the rock and sneak by the guards.";
	std::string FIGHT_NO_SWORD_RESULT = "You die. Game over.";
	std::string FIGHT_SWORD_RESULT = "You use the sword to beat them, but now you're injured.";
	// Part 2
	std::string CRATE_TTC_RESULT = "You see a crate! You take the key inside. Time to crate ";
	std::string CRATE_RESULT = "You've returned to the crate.";
	std::string DEEPER_RESULT = "There is no turning back now.";
	std::string FOUND_OAR_RESULT = "You see and pick up an oar.";
	std::string NOTHING_ELSE_RESULT = "There is nothing to see here.";
	std::string SHIP_RESULT = "You beat them and reach the ship.";
	std::string FOREST_RESULT = "You are in a forest.";
	std::string FOREST_CREW_LOCKED_RESULT = "Your crewmate is locked up.";
	std::string FOREST_CREW_FREE_RESULT = "Your crewmate is locked up. You use your key to free him.";
	std::string COAST_RETURN_RESULT = "You are back on the coast.";
	std::string DEEP_RETURN_RESULT = "You are back in the deep woods.";
	std::string RAFT_NO_OAR_RESULT = "You have found a raft but have nothing to sail with.";
	std::string RAFT_OAR_RESULT = "You have found a raft and use the oar.";
};
