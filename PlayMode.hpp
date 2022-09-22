#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>
#include <hb.h>
#include <hb-ft.h>
#include <freetype/freetype.h>

#include <vector>
#include <set>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void render_at(std::string txt, uint32_t x, uint32_t y);

	//----- game state -----

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// Text shaping
	FT_Library ft_library;
	FT_Face ft_face;
	hb_font_t* hb_font;

	// Game state
	enum Location {
		PRISON,
		DUNGEON,
		TABLE,
		GUARDS,
		COAST,
	} current_location;

	enum Choice {
		LEFT,
		RIGHT,
		NONE
	} current_choice;

	enum Item {
		SWORD,
		ROCK,
		SHOVEL
	};

	std::set<Item> items;
	
	std::string message;
	std::string result;
	std::string left_choice;
	std::string right_choice;

	bool past_guard = false;
	bool injured = false;

	// Location messages
	std::string IN_PRISON_MESSAGE = "You are in the prison. What do you do to escape?";
	std::string AT_TABLE_MESSAGE = "You are at the table. Where do you go?";
	std::string NEAR_GUARDS_MESSAGE = "You are near the guards. What do you do?"; 
	std::string DUNGEON_MESSAGE = "You are in the dungeon. Where do you go?";
	std::string COAST_MESSAGE = "You are on the coast. What do you do?";

	// Choices
	std::string GIVE_UP_CHOICE = "Give up.";
	std::string NO_CHOICE = "";
	// Left choices
	std::string DIG_TUNNEL_CHOICE = "Dig a tunnel.";
	std::string ASK_NICELY_CHOICE = "Ask him nicely to let you go.";
	std::string LOOK_AROUND_CHOICE = "Look around.";
	std::string TURN_LEFT_CHOICE = "Turn left.";
	std::string TRY_RIGHT_CHOICE = "Go right this time.";
	std::string CHARGE_CHOICE = "Charge!";

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

	
	//camera:
	Scene::Camera *camera = nullptr;

};
