#include "Patcher.h"
#include <thread>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iomanip>

void Patcher::Out(std::string msg)
{
#ifdef DEBUG
	std::cout << msg << "\n";
#endif // DEBUG

}
std::string floatToStringWithPrecision(float value, int precision) {
	std::ostringstream out;
	out << std::fixed << std::setprecision(precision) << value;
	return out.str();
}

void Patcher::Run()
{
#ifdef DEBUG
	Patcher::Out("Select an option");
	Patcher::Out("Run - 1");
	Patcher::Out("Compile GFX - 2");

	std::string opt = "";
	std::cin >> opt;

	if (opt == "2")
	{
		// gfx compiler..

		return;
	}
#endif
	std::thread(&Patcher::FetchNews, this).detach();
	std::thread(&Patcher::GetRemoteVersion, this).detach();

	this->GetLocalVersion();

	// TODO show why it's closing.. popup window?
	if (!this->font.loadFromFile("data/img/font.ttf"))
		return;

	for (int i = 0; i <= 5; i++)
	{
		if (!this->textures[i].loadFromFile("data/img/" + std::to_string(i)+".png"))
			return;

		this->sprites[i].setTexture(this->textures[i], true);
	}

	this->SortUIElements();


	sf::RenderWindow window(sf::VideoMode(this->uiWidth, this->uiHeight), this->uiName, sf::Style::Close);

	window.setFramerateLimit(60);

	this->callDraw = true;
	while (window.isOpen())
	{

		this->Tick();
		sf::Event event;
		while (window.pollEvent(event) && window.hasFocus())
		{
			if (event.type == sf::Event::Closed) {
				window.close();
			}
			else if (event.type == sf::Event::MouseMoved)
			{
				sf::Vector2i mousePos = sf::Mouse::getPosition(window);
				this->callDraw = true;

				this->UpdateButtonHover(mousePos);
			}
		}


		if (this->callDraw)
		{
			window.clear(sf::Color(25,25,25,25));
			
			window.draw(this->sprites[GFX::BG]);
			window.draw(this->sprites[GFX::INFOBOX]);
			window.draw(this->sprites[GFX::BUTTON_UPDATE]);
			window.draw(this->sprites[GFX::BUTTON_PLAY]);

			// texts

			window.draw(this->infoTitle);

			std::lock_guard<std::mutex> Lock(this->mtx);
			for (auto& txt : this->newsText)
			{
				if (txt.getGlobalBounds().height + txt.getPosition().x >= this->sprites[GFX::INFOBOX].getPosition().x + this->sprites[GFX::INFOBOX].getGlobalBounds().height - 30)
					continue;

				window.draw(txt);
			}
			window.display();
		}
		this->callDraw = false;
	}
}
void Patcher::SortUIElements()
{
	// resize bg
	this->sprites[GFX::BG].setTextureRect(sf::IntRect(0, 0, static_cast<int>(this->uiWidth), static_cast<int>(this->uiHeight)));

	//place info box
	this->sprites[GFX::INFOBOX].setPosition(50, 180);

	// set update btn as not set in initial file load loop
	this->sprites[GFX::BUTTON_UPDATE].setTexture(this->textures[GFX::BUTTON_PLAY], true);
	// buttons
	this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 0, 89, 27));

	this->sprites[GFX::BUTTON_UPDATE].setTextureRect(sf::IntRect(89, 0, 89, 27));

	this->sprites[GFX::BUTTON_UPDATE].setPosition(100, 500);
	this->sprites[GFX::BUTTON_PLAY].setPosition(300, 500);


	this->infoTitle.setFillColor(sf::Color(202, 186, 131));
	this->infoTitle.setCharacterSize(20);
	this->infoTitle.setOutlineColor(sf::Color::Black);
	this->infoTitle.setOutlineThickness(1.f);
	this->infoTitle.setFont(this->font);
	this->infoTitle.setPosition(sf::Vector2f(135, 200));
	this->infoTitle.setString("Fetching News..");

	this->infoTitleOriginal = "Fetching News..";

}
void Patcher::UpdateButtonHover(sf::Vector2i pos)
{

	if (this->sprites[GFX::BUTTON_PLAY].getGlobalBounds().contains(sf::Vector2f(pos.x, pos.y)))
	{
		this->buttonHover[GFX::BUTTON_PLAY] = true;
		this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 27, 89, 27));
	}
	else
	{
		this->buttonHover[GFX::BUTTON_PLAY] = false;
		this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 0, 89, 27));
	}

	if (this->sprites[GFX::BUTTON_UPDATE].getGlobalBounds().contains(sf::Vector2f(pos.x, pos.y)))
	{
		this->buttonHover[GFX::BUTTON_UPDATE] = true;
		this->sprites[GFX::BUTTON_UPDATE].setTextureRect(sf::IntRect(89, 27, 89, 27));
	}
	else
	{
		this->buttonHover[GFX::BUTTON_UPDATE] = false;
		this->sprites[GFX::BUTTON_UPDATE].setTextureRect(sf::IntRect(89, 0, 89, 27));
	}
}
void Patcher::GetLocalVersion()
{
	std::ifstream versionFile("data/img/version.txt");

	try
	{
		if (versionFile.good() && versionFile.is_open())
		{
			std::string line = "";

			if (std::getline(versionFile, line))
			{
				this->localVersion = std::stof(line);
			}
		}
	}
	catch (...)
	{
#ifdef DEBUG
		std::cout << "Failed to load local version\n";
#endif // DEBUG

	}
}
void Patcher::GetRemoteVersion()
{
	sf::Http http(this->siteUrl);

	sf::Http::Request listRequest;
	listRequest.setMethod(sf::Http::Request::Get); // Use GET method
	listRequest.setUri("eo/version.txt"); // Specify the URI of the webpage
	listRequest.setHttpVersion(1, 1);

	// Mimic browser-like headers
	listRequest.setField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36");
	listRequest.setField("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	listRequest.setField("Accept-Language", "en-US,en;q=0.9");
	listRequest.setField("Connection", "keep-alive");




	sf::Http::Response listResponse = http.sendRequest(listRequest);

	if (listResponse.getStatus() == sf::Http::Response::Ok)
	{
		std::istringstream responseStream(listResponse.getBody());
		std::string line;

		if(std::getline(responseStream, line))
		{
			this->remoteVersion = std::stof(line);

			if (this->localVersion < this->remoteVersion)
				this->state = States::UPDATE_FOUND;
		}
	}
}
void Patcher::SetLocalVersion()
{
}
void Patcher::Tick()
{

	if (this->state == States::UPDATE_FOUND)
	{
		// this will change to work on the update button


		if (this->localVersion < 0.0)
			this->localVersion = 0.1;
		std::lock_guard<std::mutex> Lock(this->mtx);
		this->infoTitle.setString("Fetching update list..");

		std::thread(&Patcher::CompileUpdateList, this).detach();
		this->state = States::COMPILING_UPDATE_LIST;
	}
	else if (this->state == States::UPDATE_LIST_COMPILED)
	{
		this->state = States::FETCHING_UPDATES;
		std::thread(&Patcher::FetchUpdates, this).detach();

	}
}
void Patcher::FetchUpdates()
{
	for (auto& patch : this->updates)
	{
		this->mtx.lock();
		this->infoTitle.setString("Downloading " + patch);
		this->callDraw = true;
		this->mtx.unlock();

		if (this->DownloadFile(this->siteUrl + "eo/EORClient/" + patch, patch))
		{
#ifdef DEBUG
			this->Out("Downloaded file: " + patch);
#endif // DEBUG

		}
	}

	this->mtx.lock();
	this->infoTitle.setString(this->infoTitleOriginal);
	this->callDraw = true;
	this->mtx.unlock();
}
void Patcher::CompileUpdateList()
{
	for (float i = this->localVersion; i <= this->remoteVersion; i += 0.1)
	{
		std::string verStr = floatToStringWithPrecision(i, 1);
#ifdef DEBUG
		std::cout << "Fetching list for version: " << verStr << "\n";
#endif // DEBUG
		
		sf::Http http(this->siteUrl);

		sf::Http::Request listRequest;
		listRequest.setMethod(sf::Http::Request::Get); // Use GET method
		listRequest.setUri("eo/patches/"+verStr+".txt"); // Specify the URI of the webpage
		listRequest.setHttpVersion(1, 1);


		// Mimic browser-like headers
		listRequest.setField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36");
		listRequest.setField("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
		listRequest.setField("Accept-Language", "en-US,en;q=0.9");
		listRequest.setField("Connection", "keep-alive");
		

		this->mtx.lock();
		this->infoTitle.setString("Fetching patchelist v" + verStr + " -> " + floatToStringWithPrecision(this->remoteVersion, 1));
		this->callDraw = true;
		this->mtx.unlock();

		sf::Http::Response listResponse = http.sendRequest(listRequest);

		if (listResponse.getStatus() == sf::Http::Response::Ok)
		{
			std::istringstream responseStream(listResponse.getBody());
			std::string line;

			std::vector<std::string> tmpVec;

			while (std::getline(responseStream, line))
			{
				bool found = false;
				for (auto& patch : this->updates)
				{
					if (patch == line)
						found = true;

				}
				std::cout << line << "\n";

				if(!found)
					tmpVec.emplace_back(line);
			}
			for (auto& patch : tmpVec)
			{
				this->updates.emplace_back(patch);
#ifdef DEBUG
				std::cout << "Adding patch: " << patch << " to the update compile list\n";
#endif // DEBUG
			}
		}
		else
		{
			this->mtx.lock();
			this->infoTitle.setString("Failed to fetch news..");
			this->mtx.unlock();
			std::cout << "Bad response\n";
		}
	}
	if (this->updates.size())
	{
#ifdef DEBUG
		std::cout << "Update list:\n";
		for (auto& patch : this->updates)
			std::cout << patch << "\n";
#endif
		this->state = States::UPDATE_LIST_COMPILED;
	}
	else
		this->state = States::READY_TO_PLAY;
	this->callDraw = true;
}
void Patcher::FetchNews()
{
	sf::Http http(this->siteUrl);
	
	sf::Http::Request listRequest;
	listRequest.setMethod(sf::Http::Request::Get); // Use GET method
	listRequest.setUri("eo/news.txt"); // Specify the URI of the webpage
	listRequest.setHttpVersion(1, 1);

	// Mimic browser-like headers
	listRequest.setField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36");
	listRequest.setField("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	listRequest.setField("Accept-Language", "en-US,en;q=0.9");
	listRequest.setField("Connection", "keep-alive");




	sf::Http::Response listResponse = http.sendRequest(listRequest);

	this->mtx.lock();
	if (listResponse.getStatus() == sf::Http::Response::Ok)
	{
		std::istringstream responseStream(listResponse.getBody());
		std::string line;

		int startX = 80;
		int startY = 230;

		while (std::getline(responseStream, line))
		{

#ifdef DEBUG
			Patcher::Out("[News] " + line);
#endif // DEBUG

			if (this->infoTitleOriginal == "Fetching news..")
			{
				this->infoTitle.setString(line);
				this->infoTitleOriginal = line;
				continue;
			}
			sf::Text txt(line, this->font, 14);
			txt.setFillColor(sf::Color(202, 186, 131));
			txt.setOutlineThickness(1.f);
			txt.setOutlineColor(sf::Color::Black);
			txt = this->WrapText(txt, 320);
			txt.setPosition(startX, startY);
			this->newsText.emplace_back(txt);

			startY += txt.getGlobalBounds().getSize().y + 10;
		}
	}
	else
	{
		this->infoTitle.setString("Failed to fetch news..");
	}
	this->mtx.unlock();

	this->callDraw = true;
}

bool Patcher::DownloadFile(const std::string& url, const std::string& outputFilename)
{
	// Parse the URL
	sf::Http::Request request;
	sf::Http http;

	// Extract the protocol, hostname, and path
	size_t protocolEnd = url.find("://");
	size_t hostStart = protocolEnd + 3;
	size_t pathStart = url.find('/', hostStart);

	std::string protocol = url.substr(0, protocolEnd);
	std::string hostname = url.substr(hostStart, pathStart - hostStart);
	std::string path = url.substr(pathStart);

	if (protocol != "http") {
#ifdef DEBUG
		std::cerr << "Only HTTP protocol is supported." << std::endl;
#endif
		return false;
	}

	// Set up the HTTP object
	http.setHost("http://" + hostname);

	// Set up the request
	request.setMethod(sf::Http::Request::Get);
	request.setUri(path);
	request.setHttpVersion(1, 1); // HTTP 1.1

	// Mimic browser-like headers
	request.setField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36");
	request.setField("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	request.setField("Accept-Language", "en-US,en;q=0.9");
	request.setField("Connection", "keep-alive");

	// Send the request and get the response
	sf::Http::Response response = http.sendRequest(request);

	if (response.getStatus() == sf::Http::Response::Ok)
	{
		// Open the output file
		std::ofstream outputFile(outputFilename, std::ios::binary);

		if (!outputFile) 
		{
#ifdef DEBUG
			std::cerr << "Failed to open the output file." << std::endl;
#endif
			return false;
		}

		// Write the response body to the file
		outputFile << response.getBody();
		outputFile.close();
#ifdef DEBUG
		std::cout << "File downloaded successfully: " << outputFilename << std::endl;
#endif
		return true;
	}
	else
	{
#ifdef DEBUG
		std::cerr << "Failed to download the file. HTTP Status: " << response.getStatus() << std::endl;
#endif
		return false;
	}
}
sf::Text Patcher::WrapText(sf::Text& text, float width)
{
	sf::FloatRect textBounds = text.getLocalBounds();
	float textWidth = textBounds.width;

	if (textWidth > width)
	{
		std::string originalString = text.getString();
		std::string wrappedString;
		std::string word;
		std::istringstream iss(text.getString());

		while (iss >> word)
		{
			sf::Text tempText = text;
			tempText.setString(wrappedString + word + " ");

			sf::FloatRect tempBounds = tempText.getLocalBounds();
			float tempWidth = tempBounds.width;

			if (tempWidth > width)
			{
				wrappedString += "\n" + word + " ";
			}
			else
			{
				wrappedString += word + " ";
			}
		}

		text.setString(wrappedString);
	}

	return text;
}
