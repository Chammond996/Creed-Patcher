#pragma once
#include "SFML/Network.hpp"
#include "SFML/Graphics.hpp"
#include "SFML/Main.hpp"
#include <mutex>
#include <iostream>


class Patcher
{
private:

	unsigned int uiWidth = 500;
	unsigned int uiHeight = 600;
	std::string uiName = "Restart Launcher";
	std::string siteUrl = "http://endless-online-restart.com/";

	//std::vector<std::string> updateList;

	enum GFX {
		BG = 0,
		INFOBOX = 1,
		BUTTON_PLAY = 2,
		BUTTON_UPDATE = 3
	};
	std::map<int, sf::Sprite> sprites;
	std::map<int, sf::Texture> textures;
	std::map<int, bool> buttonHover;

	sf::Text infoTitle;
	std::string infoTitleOriginal;

	sf::Font font;

	bool callDraw = false;

	std::vector<sf::Text> newsText;

	float localVersion = -1.0;
	float remoteVersion = -1.0;
	std::vector<std::string> updates;

	bool DownloadFile(const std::string& url, const std::string& outputFilename);

	void GetLocalVersion();
	void SetLocalVersion(float value);
	void GetRemoteVersion();
	void CompileUpdateList();
	void StartUpdate();
	void PlayGame();
	void Tick();
	void FetchUpdates();
	void CheckButtonClick(int x, int y);
	void CompileGFXPack();
	void LoadGFX();
	enum States {
		CHECKING_FOR_UPDATE = 0,
		UPDATE_FOUND = 1,
		COMPILING_UPDATE_LIST = 2,
		UPDATE_LIST_COMPILED = 3,
		FETCHING_UPDATES = 4,
		READY_TO_PLAY = 5
	};
	States state;
	bool terminate = false;

public:

	Patcher() {};

	std::mutex mtx;

	void FetchInfo();
	void Out(std::string msg);
	void Run();
	void SortUIElements();
	void UpdateButtonHover(sf::Vector2i pos);
	sf::Text WrapText(sf::Text& text, float width);
};

