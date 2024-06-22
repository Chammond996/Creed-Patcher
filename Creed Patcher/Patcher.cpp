#include "Patcher.h"
#include <thread>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <Windows.h>

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
	this->terminate = false;
#ifdef DEBUG
	Patcher::Out("Select an option");
	Patcher::Out("Run - 1");
	Patcher::Out("Compile GFX - 2");

	std::string opt = "";
	std::cin >> opt;

	if (opt == "2")
	{
		std::string msg = "This will write the graphics from ui_dev/ into\n";
		msg += ".dat gfx files. Once you have checked you have:\n";
		msg += "0.png = BG, 1.png = infoboard, 2.png = buttons, 3.icon.png = icon(for window)..\n";
		msg += "Press c to continue";
		Patcher::Out(msg);

		std::cin >> msg;

		if (msg == "c")
		{
			//compile
			this->CompileGFXPack();
		}
		else
		{
			Patcher::Out("Exiting compiler");
			sf::sleep(sf::seconds(2));
		}

		return;
	}
#endif

	this->GetLocalVersion();

	// TODO show why it's closing.. popup window?
	if (!this->font.loadFromFile("data/font.ttf"))
		return;

	this->LoadGFX();
	
	bool fetchData = false;

	std::thread(&Patcher::FetchInfo, this).detach();

	this->SortUIElements();

	sf::RenderWindow window(sf::VideoMode(this->uiWidth, this->uiHeight), this->uiName, sf::Style::Close);


	sf::Image icon;
	icon = this->textures[3].copyToImage();

	window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

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
			else if (event.type == sf::Event::MouseButtonReleased)
			{
				if (event.mouseButton.button == sf::Mouse::Left)
				{
					this->CheckButtonClick(event.mouseButton.x, event.mouseButton.y);
				}
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

			this->mtx.lock();


			window.draw(this->infoTitle);
			
			for (auto& txt : this->newsText)
			{
				if (txt.getGlobalBounds().height + txt.getPosition().x >= this->sprites[GFX::INFOBOX].getPosition().x + this->sprites[GFX::INFOBOX].getGlobalBounds().height - 30)
					continue;

				window.draw(txt);
			}
			this->mtx.unlock();

			window.display();
		}
		this->callDraw = false;

		if (!fetchData)
		{
			fetchData = true;
			//this->FetchInfo();
		}
		if (this->terminate)
			window.close();
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
	this->infoTitle.setString("Checking for updates!");

	this->infoTitleOriginal = "";

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
		if (this->state != States::READY_TO_PLAY)
		{
			this->buttonHover[GFX::BUTTON_PLAY] = false;
			this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 0, 89, 27));
		}
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
	std::ifstream versionFile("data/version.txt");

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
	return;
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
			{
				this->state = States::UPDATE_FOUND;
				this->infoTitle.setString("patch Found!  -" + floatToStringWithPrecision(this->remoteVersion, 1)+"-");
			}
			else
			{
				this->state = States::READY_TO_PLAY;
			}
		}
	}
	this->callDraw = true;
}
void Patcher::SetLocalVersion(float value)
{
	std::ofstream file("data/version.txt", std::ios::trunc);
	file << value;
	file.close();
}
void Patcher::Tick()
{
	if (this->state == States::UPDATE_LIST_COMPILED)
	{
		this->state = States::FETCHING_UPDATES;
		std::thread(&Patcher::FetchUpdates, this).detach();
	}

}
void Patcher::FetchUpdates()
{
	for (auto& patch : this->updates)
	{
		this->infoTitle.setString("Downloading " + patch);
		this->callDraw = true;

		if (this->DownloadFile(this->siteUrl + "eo/EORClient/" + patch, patch))
		{
#ifdef DEBUG
			this->Out("Downloaded file: " + patch);
#endif // DEBUG

		}
	}
	this->mtx.lock();
	this->infoTitle.setString(this->infoTitleOriginal);
	this->state = States::READY_TO_PLAY;
	this->SetLocalVersion(this->remoteVersion);
	this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 27, 89, 27));

	this->callDraw = true;
	this->mtx.unlock();
}
void Patcher::StartUpdate()
{
	if (this->localVersion < 0.0)
		this->localVersion = 0.1;

	this->mtx.lock();
	this->infoTitle.setString("Fetching update list..");
	this->mtx.unlock();

	std::thread(&Patcher::CompileUpdateList, this).detach();
	this->state = States::COMPILING_UPDATE_LIST;
}
void Patcher::PlayGame()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process
	if (!CreateProcess(
		L"endless.exe", // Path to the program
		NULL,         // Command line (if needed)
		NULL,         // Process handle not inheritable
		NULL,         // Thread handle not inheritable
		FALSE,        // Set handle inheritance to FALSE
		0,            // No creation flags
		NULL,         // Use parent's environment block
		NULL,         // Use parent's starting directory
		&si,          // Pointer to STARTUPINFO structure
		&pi)          // Pointer to PROCESS_INFORMATION structure
		) 
	{
#ifdef DEBUG
		std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
#endif
		return;
	}

	this->terminate = true;
}
void Patcher::CheckButtonClick(int x, int y)
{
	if (this->sprites[GFX::BUTTON_UPDATE].getGlobalBounds().contains(sf::Vector2f(x, y)))
	{
		if (this->state == States::UPDATE_FOUND)
		{
			this->StartUpdate();
		}
	}
	else if (this->sprites[GFX::BUTTON_PLAY].getGlobalBounds().contains(sf::Vector2f(x, y)))
	{
		if (this->state == States::READY_TO_PLAY)
		{
			this->PlayGame();
		}
	}
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
#ifdef DEBUG
				std::cout << line << "\n";
#endif

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
			this->infoTitle.setString("Failed to fetch patchlist.");
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
	{
		this->state = States::READY_TO_PLAY;
		this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 27, 89, 27));
	}
	this->callDraw = true;
}
void Patcher::FetchInfo()
{
	sf::Http http(this->siteUrl);
	
	sf::Http::Request listRequest;
	listRequest.setMethod(sf::Http::Request::Get); // Use GET method
	listRequest.setUri("eo/info.txt"); // Specify the URI of the webpage
	listRequest.setHttpVersion(1, 1);

	// Mimic browser-like headers
	listRequest.setField("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.114 Safari/537.36");
	listRequest.setField("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9");
	listRequest.setField("Accept-Language", "en-US,en;q=0.9");
	listRequest.setField("Connection", "keep-alive");




	sf::Http::Response listResponse = http.sendRequest(listRequest);

	if (listResponse.getStatus() == sf::Http::Response::Ok)
	{
		this->mtx.lock();

		std::istringstream responseStream(listResponse.getBody());
		std::string line;

		int startX = 80;
		int startY = 230;

		int c = 0;
		while (std::getline(responseStream, line))
		{
			if (c == 0)//version
			{
				try
				{
					this->remoteVersion = std::stof(line);
				}
				catch (...)
				{
					this->remoteVersion = 0.0;
					c++;
					continue;
				}
				if (this->localVersion < this->remoteVersion)
				{
					this->state = States::UPDATE_FOUND;
					this->infoTitle.setString("Patch Found! (" + floatToStringWithPrecision(this->remoteVersion, 1)+")");
				}
				else
				{
					this->state = States::READY_TO_PLAY;
					this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 27, 89, 27));
				}
			}
			else if (c > 0) // news
			{
#ifdef DEBUG
				Patcher::Out("[News] " + line);
#endif // DEBUG

				if (this->infoTitleOriginal == "")
				{
					this->infoTitleOriginal = line;
					if (this->state == States::READY_TO_PLAY)
					{
						this->infoTitle.setString(line);
						this->sprites[GFX::BUTTON_PLAY].setTextureRect(sf::IntRect(0, 27, 89, 27));
					}

					continue;
				}
				sf::Text txt;
				txt.setCharacterSize(14);
				txt.setString(line);
				txt.setFont(this->font);
				txt.setFillColor(sf::Color(202, 186, 131));
				txt.setOutlineThickness(1.f);
				txt.setOutlineColor(sf::Color::Black);
				txt = this->WrapText(txt, 320);
				txt.setPosition(startX, startY);
				this->newsText.emplace_back(txt);

				startY += txt.getGlobalBounds().getSize().y + 10;
			}
			c++;
		}
		this->mtx.unlock();
	}
	else
	{
		this->mtx.lock();
		this->infoTitle.setString("Failed to fetch news..");
		this->mtx.unlock();
	}

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
	sf::Http::Response response = http.sendRequest(request, sf::Time(sf::seconds(5)));

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
}void Patcher::CompileGFXPack()
{
	std::ofstream gfxPack("Data/patcher.dat", std::ios::binary | std::ios::trunc);

	if (gfxPack.good() && gfxPack.is_open())
	{
		for (int i = 0; i <= 3; i++)
		{
			sf::Image img;
			std::string path = "ui_dev/" + std::to_string(i)+".png";
			if (img.loadFromFile(path))
			{
				sf::Uint32 sizeX = img.getSize().x, sizeY = img.getSize().y;
				gfxPack.write(reinterpret_cast<char*>(&sizeX), sizeof(sf::Uint32));
				gfxPack.write(reinterpret_cast<char*>(&sizeY), sizeof(sf::Uint32));

				Patcher::Out("Compiling " + path);
				for (int x = 0; x < sizeX; x++)
				{
					for (int y = 0; y < sizeY; y++)
					{
						sf::Color px = img.getPixel(x, y);
						sf::Uint8 r = px.r, g = px.g, b = px.b, a = px.a;

						gfxPack.write(reinterpret_cast<char*>(&a), sizeof(sf::Uint8));
						gfxPack.write(reinterpret_cast<char*>(&g), sizeof(sf::Uint8));
						gfxPack.write(reinterpret_cast<char*>(&b), sizeof(sf::Uint8));
						gfxPack.write(reinterpret_cast<char*>(&r), sizeof(sf::Uint8));
					}
				}
			}
			else
			{
				Patcher::Out("Failed to load " + std::to_string(i) + ".png for compile, exiting..");
				sf::sleep(sf::seconds(3));
				return;
			}
		}


		gfxPack.close();
	}
}

void Patcher::LoadGFX()
{
	std::ifstream gfxPack("data/patcher.dat", std::ios::binary);
	if (gfxPack.good() && gfxPack.is_open())
	{
		for (int i = 0; i <= 3; i++)
		{
			sf::Uint32 sizeX = 0, sizeY = 0;

			gfxPack.read(reinterpret_cast<char*>(&sizeX), sizeof(sf::Uint32));
			gfxPack.read(reinterpret_cast<char*>(&sizeY), sizeof(sf::Uint32));

			if (sizeX && sizeY > 0)
			{
				sf::Image img;
				img.create(sizeX, sizeY);

				for (int x = 0; x < sizeX; x++)
				{
					for (int y = 0; y < sizeY; y++)
					{
						sf::Uint8 r = 0, g = 0, b = 0, a = 0;

						gfxPack.read(reinterpret_cast<char*>(&a), sizeof(sf::Uint8));
						gfxPack.read(reinterpret_cast<char*>(&g), sizeof(sf::Uint8));
						gfxPack.read(reinterpret_cast<char*>(&b), sizeof(sf::Uint8));
						gfxPack.read(reinterpret_cast<char*>(&r), sizeof(sf::Uint8));

						img.setPixel(x, y, sf::Color(r, g, b, a));
					}
				}
				this->textures[i].loadFromImage(img);
				this->sprites[i].setTexture(this->textures[i]);
			}

		}

		gfxPack.close();
	}
}
