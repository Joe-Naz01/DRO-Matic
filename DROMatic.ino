#include <DS3231.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>
#include <StandardCplusplus.h>
#include <vector>
#include <ArduinoJson\ArduinoJson.h>
DS3231  rtc(SDA, SCL);
using namespace std;


//Global vars
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
int Key;
const int chipSelect = 4;
unsigned long previousMillis = 0;        // will store last time

String cropName;
String renameArry[15] = { "", "", "", "", "", "", "", "", "", "", "", "", "", "" };
String screenName;
bool enterHomeFlag = false;
long exitHomeWait = 0;
int cursorX = 0;
int cursorY = 0;
int menuIndex = 0;
int totalChannels = 0;
int minPPM = 1200;
int maxPPM = 1400;

int hour;
int minute;
int sec;
int day;
int month;
int year;
int days[12] = { 31, ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 28 : 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int currentChannelIndex;
int currentSessionIndex;
int currentAlphaIndex = 0;
const char* alphabet[37] = { 
	" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", 
	"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", 
	"1", "2", "3", "4", "5", "6", "7", "8", "9"
};
const char* displayNames[17][3] = {
	{ "SYSCONF", "SYSTEM", "SETTINGS" },
	{ "CHANNELS", "SYSTEM", "CHANNELS" },
	{ "DATETIME", "DATE & TIME", "CONFIGURATION" },
	{ "CHCONF", "CHANNEL", "SETTINGS" },
	{ "CHNUM", "NUMBER OF", "CHANNELS" },
	{ "PPM", "EC/PPM RANGE", "CONFIGURATION" },
	{ "OPEN", "LOAD CROP" },
	{ "NEW", "NEW CROP" },
	{ "DELETE", "DELETE CROP" },
	{ "CHDOSES", "NUMBER OF", "SESSIONS" },
	{ "CHSIZE", "SIZE(ml) OF", "CHANNEL" },
	{ "CHCALIB", "CHANNEL", "CALIBRATION" },
	{ "SESSIONS", "CHANNEL", "SESSIONS" },
	{ "AMT", "SESSION", "AMOUNT(ml)" },
	{ "STR", "SESSION START", "DATE & TIME" },
	{ "DLY", "SESSION DELAY", "CONFIGURATION" },
	{ "RPT", "SESSION REPEAT", "CONFIGURATION" }
};
const char* months[12] = { "Jan", "Feb", "Mar", "Apl", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
String suffix;
String displayHour;
String displayMin;
String displayDay;

byte upArrow[8] = {
	B00000,
	B00100,
	B01110,
	B11111,
	B00100,
	B00100,
	B00100,
	B00000
};
byte downArrow[8] = {
	B00000,
	B00100,
	B00100,
	B00100,
	B11111,
	B01110,
	B00100,
	B00000
};

vector<String> menus;
vector<String> menusHistory;
vector<vector<vector<int>>> matrix;

//Data passing functions for core and crop
JsonObject& getCoreData(){
	StaticJsonBuffer<512> buffer;
	File core = SD.open("core.dro");
	return buffer.parseObject(core.readString());
}

void setCoreData(JsonObject& data){
	char buffer[128];
	File core = SD.open("core.dro", O_WRITE | O_TRUNC);
	data.printTo(buffer, sizeof(buffer));
	core.print(buffer);
	core.close();
}

JsonObject& getCropData(){
	StaticJsonBuffer<512> buffer;
	File crop = SD.open(getCropName() + "/crop.dro");
	return buffer.parseObject(crop.readString());
}

JsonObject& parseCropData(File cropFile){
	StaticJsonBuffer<512> buffer;
	return buffer.parseObject(cropFile.readString());
}

String getCropName(){
	JsonObject& data = getCoreData();
	return data["crop"];
}

void setCropData(JsonObject& data){
	char buffer[128];
	File crop = SD.open(cropName + "/crop.dro", O_WRITE | O_TRUNC);
	data.printTo(buffer, sizeof(buffer));
	crop.print(buffer);
	crop.close();
}

void setup()
{
	lcd.createChar(0, upArrow);
	lcd.createChar(1, downArrow);
	lcd.begin(16, 2);
	Serial.begin(9600);	
	rtc.begin();
	captureDateTime();

	if (!SD.begin(53)){
		lcd.print("SD Card Required");
		lcd.setCursor(0,1);
		lcd.print("Insert And Rest");
	}else{
		coreInit();
	}
}

void loop()
{
	Key = analogRead(0);
	unsigned long currentMillis = millis();
	if (Key >= 0 && Key <= 650){
		previousMillis = currentMillis;
	}
	if (currentMillis - previousMillis >= 10000 && screenName == "") {
		previousMillis = currentMillis;
		openHomeScreen();
	}

	if (Key == 0){
		//Right
		if (screenName == "NewCrop"){
			matrix = {
				{ { 0, 15 } },
				{ { 11, 11 } }
			};
			cursorX = cursorX + 1;
			screenMatrix();
			renameCrop(NULL);
			lcd.blink();
			delay(250);
		}
		if (screenName == "DATETIME"){
			matrix = {
				{ { 1, 1 }, { 4, 4 }, { 6, 6 }, { 10, 10 }, {13, 13} },
				{ { 3, 3 }, { 6, 6 }, {13, 13} }
			};
			cursorX = cursorX + 1;
			screenMatrix();
			delay(250);
		}
		if (screenName == "PPM"){
			matrix = {
				{ { 3, 3 }, { 8, 8 } },
				{ { 1, 1 }, { 13, 13 } }
			};
			cursorX = cursorX + 1;
			screenMatrix();
			delay(250);
		}
	}
	if (Key == 99) {
		//Up
		if (screenName == ""){
			scrollMenus(1);
		}
		if (screenName == "NewCrop"){
			renameCrop(1);
			delay(250);
		}
		if (screenName == "PPM"){
			setPPMRangeValues(1);
			delay(250);
		}
		if (screenName == "DATETIME"){
			setDateTime(1);
			delay(250);
		}
		if (screenName == "CHNUM"){

		}
	}
	if (Key == 255){
		//Down
		if (screenName == ""){
			scrollMenus(-1);
		}
		if (screenName == "NewCrop"){
			renameCrop(-1);
			delay(250);
		}
		if (screenName == "PPM"){
			setPPMRangeValues(-1);
			delay(250);
		}
		if (screenName == "DATETIME"){
			setDateTime(-1);
			delay(250);
		}
		if (screenName == "CHNUM"){
			
		}
	}
	if (Key == 408) {
		//Left
		if (screenName == ""){
			menusHistory.pop_back();
			menuIndex = 0;
			File prevLvl = SD.open(cropName+"/"+getMenuHistory());
			getDirectoryMenus(prevLvl);
			lcd.clear();
			prevLvl.close();
			printDisplayNames(menus.front());
			printScrollArrows();
			delay(350);
		}
		if (screenName == "NewCrop"){
			matrix = {
				{ { 0, 15 },
				{ 11, 11 } }
			};
			cursorX = cursorX - 1;
			screenMatrix();
			delay(250);
		}
		if (screenName == "DATETIME"){
			matrix = {
				{ { 1, 1 }, { 4, 4 }, { 6, 6 }, { 10, 10 }, { 13, 13 } },
				{ { 3, 3 }, { 6, 6 }, { 13, 13 } }
			};
			cursorX = cursorX - 1;
			screenMatrix();
			delay(250);
		}
		if (screenName == "PPM"){
			matrix = {
				{ { 3, 3 }, { 8, 8 } },
				{ { 1, 1 }, { 13, 13 } }
			};
			cursorX = cursorX - 1;
			screenMatrix();
			delay(250);
		}
	}
	if (Key == 639) {
		//Select
		if (screenName == ""){
			File nextLvl = SD.open(cropName + "/" + getMenuHistory() + "/" + menus[menuIndex]);
			menusHistory.push_back(menus[menuIndex]);
			getDirectoryMenus(nextLvl);
			if (menus.size() > 0){
				menuIndex = 0;
				lcd.clear();
				nextLvl.close();
				printDisplayNames(menus.front());
				printScrollArrows();
			}else{
				//screenName = menus[menuIndex];
				screenName = menusHistory.back();
				if (screenName == "DATETIME"){
					lcd.blink();
					lcd.clear();
					lcd.home();
					captureDateTime();
					lcd.print(String(displayHour) + ":" + String(displayMin) + "AM " + String(months[month]) + " " + String(displayDay));
					lcd.setCursor(0, 1);
					lcd.print(String(year)+" <back> <ok>");
					lcd.setCursor(1, 0);
					cursorX = 1;
					cursorY = 0;
				}
				if (screenName == "PPM"){
					lcd.blink();
					lcd.clear();
					lcd.home();
					lcd.print(String(minPPM) + "-" + String(maxPPM) + " EC/PPM");
					lcd.setCursor(0, 1);
					lcd.print("<back>      <ok>");
					lcd.setCursor(3, 0);
					cursorX = 3;
				}
				if (screenName == "CHNUM"){
					lcd.blink();
					lcd.clear();
					lcd.home();
					lcd.print(10 + " CHANNELS");
					lcd.setCursor(10,1);
					lcd.print("<save>");
					lcd.setCursor(1,0);
				}
			}
			delay(350);
		}
		//Saves
		if (screenName == "NewCrop"){
			if (cursorX == 11 && cursorY == 1){
				buildCrop();
			}
		}
		if (screenName == "PPM"){
			if (cursorX == 13 && cursorY == 1){
				JsonObject& data = getCropData();
				data["minPPM"] = minPPM;
				data["maxPPM"] = maxPPM;
				setCropData(data);
			}
			if (cursorX == 1 || cursorX == 13 && cursorY == 1){
				lcd.clear();
				screenName = "";
				lcd.home();
				lcd.print(menus[menuIndex]);
				printScrollArrows();
			}
		}
		if (screenName == "DATETIME"){
			if (cursorX == 13 && cursorY == 1){
				rtc.setTime(hour, minute, sec);
				rtc.setDate(day, month, year);
			}
			if (cursorX == 6 || cursorX == 13 && cursorY == 1){
				menusHistory.pop_back();
				menuIndex = 0;
				screenName = "";
				File prevLvl = SD.open(cropName + "/" + getMenuHistory());
				getDirectoryMenus(prevLvl);
				lcd.clear();
				lcd.noBlink();
				prevLvl.close();
				printDisplayNames(menus.front());
				printScrollArrows();
				delay(350);
			}
			
		}
	}

}

void openHomeScreen(){
	captureDateTime();
	lcd.clear();
	lcd.print(displayHour + ":" + displayMin + "PM " + months[rtc.getTime().mon] + " " + displayDay);
	lcd.setCursor(0, 1);
	lcd.print("PPM:1275 PH:6.0");
	lcd.home();
	lcd.noBlink();
}

void screenMatrix(){
	int row0Size = matrix[0].size();
	int row1Size = matrix[1].size();

	//Check Top Row
	if (cursorY == 0){
		if (cursorX < matrix.front().front().front()){
			cursorX = matrix.back().back().back();
			cursorY = 1;
		}
		else if (cursorX > matrix.front().back().back()){
			cursorY = 1;
			cursorX = matrix.back().front().front();
		}
		for (int i = 0; i < row0Size; i++){ //top columns
			if (cursorX < matrix.front()[i].front() && cursorX > matrix.front()[i - 1].back() && Key == 408 && cursorY == 0){
				cursorX = matrix.front()[i - 1].back();
			}
			if (cursorX > matrix.front()[i].back() && cursorX < matrix.front()[i + 1].front() && Key == 0 && cursorY == 0){
				cursorX = matrix.front()[i + 1].front();
			}
		}
	}
	else if (cursorY == 1){
		if (cursorX < matrix.back().front().front()){
			cursorX = matrix.front().back().back();
			cursorY = 0;
		}
		else if (cursorX > matrix.back().back().back()){
			cursorY = 0;
			cursorX = matrix.front().front().front();
		}
		for (int i = 0; i < row1Size; i++){ //top columns
			if (cursorX < matrix.back()[i].front() && cursorX > matrix.back()[i - 1].back() && Key == 408 && cursorY == 1){
				cursorX = matrix.back()[i - 1].back();
			}
			if (cursorX > matrix.back()[i].back() && cursorX < matrix.back()[i + 1].front() && Key == 0 && cursorY == 1){
				cursorX = matrix.back()[i + 1].front();
			}
		}
	}
	lcd.setCursor(cursorX, cursorY);
}

int getChannelsTotal(){
	JsonObject& data = getCropData();
	return data["totalChannels"];
}

int getSessionTotal(){
	StaticJsonBuffer<512> buffer;
	File session = SD.open(getCropName() + "/Channels/Channel" + String(currentChannelIndex) + "/channel.dro");
	JsonObject& data = buffer.parseObject(session.readString());
	return data["sessionsTotal"];
}

void coreInit(){
	File coreFile = SD.open("core.dro");
	if (coreFile){ //load core settings / crop
		JsonObject& coreData = getCoreData();
		String name = coreData["crop"];
		cropName = name;

		if (SD.exists(cropName)){
			//Loading up exisiting core file's crop directory
			screenName = "";
			File cropFile = SD.open("/" + cropName);
			getDirectoryMenus(cropFile);
			JsonObject& cropData = parseCropData(cropFile);
			totalChannels = cropData["totalChannels"];
			cropFile.close();
			openHomeScreen();
		}else{
			//we have core file with crop, but no crop directory. //VERY CORNER CASE!
			startNewCrop();
		}
	}else{ //else, setup new crop
		char buffer[512];
		StaticJsonBuffer<512> coreBuffer;
		JsonObject& settings = coreBuffer.createObject();
		coreFile = SD.open("core.dro", FILE_WRITE);
		settings["crop"];
		settings.printTo(buffer, sizeof(buffer));
		coreFile.print(buffer);
		coreFile.close();
		lcd.print("New drive Found!");
		lcd.setCursor(0, 1);
		lcd.print(" Please wait... ");
		delay(2000);
		lcd.clear();
		lcd.home();
		lcd.print("Setup new crop");
		delay(2000);
		lcd.clear();
		startNewCrop();
	}
	coreFile.close();
}

void startNewCrop(){
	screenName = "NewCrop";
	lcd.clear();
	lcd.setCursor(0, 1);
	lcd.print("Crop Name <done>");
	lcd.home();
	lcd.blink();
}

void scrollAlpha(int dir){
	currentAlphaIndex = currentAlphaIndex + dir;
	if (currentAlphaIndex > 36){
		currentAlphaIndex = 0;
	}else if (currentAlphaIndex < 0){
		currentAlphaIndex = 36;
	}
	lcd.print(alphabet[currentAlphaIndex]);
	lcd.setCursor(cursorX, cursorY);
	delay(50);
}

void scrollMenus(int dir){
	menuIndex = menuIndex + dir;
	int currentSize = menus.size() - 1;
	if (menuIndex < 0){
		menuIndex = currentSize;
	}else if (menuIndex > currentSize){
		menuIndex = 0;
	}
	lcd.clear();
	printDisplayNames(menus[menuIndex]);
	printScrollArrows();
	delay(350);
}

void printScrollArrows(){
	lcd.setCursor(15, 0);
	lcd.write(byte(0));
	lcd.setCursor(15, 15);
	lcd.write(byte(1));
	lcd.home();
}

void getDirectoryMenus(File dir) {
	menus.clear();
	while (true) {
		File entry = dir.openNextFile();
		if (!entry) {
			// no more files
			break;
		}
		if (entry.isDirectory()){
			menus.push_back(entry.name());
		}
		entry.close();
	}
}

int getDirectoryMenusSize(File dir){
	int count = 0;
	while (true){
		File entry = dir.openNextFile();
		if (!entry){
			// no more files
			break;
		}
		if (entry.isDirectory()){
			count++;
		}
	}
	return count;
}

String getMenuHistory(){
	String concat;
	for (int i = 0; i < menusHistory.size(); i++){
		concat = concat +"/"+ menusHistory.at(i);
	}
	return concat;
}

void printDisplayNames(String menu){
	lcd.home();
	bool hasMatch = false;
	const int isChannel = strstr(menu.c_str(), "SYSCH") != NULL;
	const int isSession = strstr(menu.c_str(), "CHSES") != NULL;
	if (isChannel){
		lcd.print("SYSTEM");
		lcd.setCursor(0, 1);
		lcd.print("CHANNEL " + String(menuIndex+1));
		lcd.home();
		hasMatch = true;
	}else if(isSession){
		lcd.print("CHANNEL");
		lcd.setCursor(0, 1);
		lcd.print("SESSION " + String(menuIndex+1));
		lcd.home();
		hasMatch = true;
	}else{
		for (int i = 0; i < sizeof(displayNames) / sizeof(*displayNames); i++){
			if (menu == displayNames[i][0]){
				hasMatch = true;
				lcd.print(displayNames[i][1]);
				if (displayNames[i][2]) {
					lcd.setCursor(0, 1);
					lcd.print(displayNames[i][2]);
				}
			}
		}
	}
	if (hasMatch == false){
		lcd.print(menu);
	}
}

void renameCrop(int dir){
	cropName = "";
	if (dir != NULL){
		scrollAlpha(dir);
		renameArry[cursorX] = alphabet[currentAlphaIndex];
	}
	//Compile/Collapse
	for (int i = 0; i < 15; i++){
		cropName = cropName + renameArry[i];
	}
	lcd.setCursor(cursorX, cursorY);
}

void makeNewFile(String path, JsonObject& data){
	char buffer[128];
	File file = SD.open(path, FILE_WRITE);
	data.printTo(buffer, sizeof(buffer));
	file.print(buffer);
	file.close();
}

void setPPMRangeValues(int dir){
	if ((dir == 1) ? cursorX == 3 : cursorX == 8){
		(dir == 1) ? minPPM = minPPM + 10 : maxPPM = maxPPM - 10;
		if ((dir == 1) ? minPPM > (maxPPM - 50) : maxPPM < (minPPM + 50)){
			(dir == 1) ? maxPPM = maxPPM + 10 : minPPM = minPPM - 10;
		}
	}
	if ((dir == 1) ? cursorX == 8 : cursorX == 3) {
		(dir == 1) ? maxPPM = maxPPM + 10 : minPPM = minPPM - 10;
	}

	lcd.clear();
	lcd.print(String(minPPM) + "-" + String(maxPPM) + " EC/PPM");
	lcd.setCursor(0, 1);
	lcd.print("<back>      <ok>");
	lcd.setCursor(cursorX, 0);
}

void setDateTime(int dir){
	int maxDaysInMonth = days[month];
	if (cursorY == 0){
		if (cursorX == 1){
			(dir == 1) ? (hour = (hour + 1 > 24) ? 1 : hour + 1) : (hour = (hour - 1 < 1) ? 24 : hour - 1);
		}
		if (cursorX == 4){
			(dir == 1) ? (minute = (minute + 1 > 59) ? 0 : minute + 1) : (minute = (minute - 1 < 0) ? 59 : minute - 1);
		}
		if (cursorX == 10){
			(dir == 1) ? (month = (month + 1 > 11) ? 0 : month + 1) : (month = (month - 1 < 0) ? 11 : month - 1);
			day = 1;
		}
		if (cursorX == 13){
			(dir == 1) ? (day = (day + 1 > maxDaysInMonth) ? 1 : day + 1) : (day = (day - 1 < 1) ? maxDaysInMonth : day - 1);
		}
	}
	else{
		if (cursorX == 3){
			year = (dir == 1)? year + 1: year - 1;
		}
	}

	lcd.clear();
	captureDateTimeDisplays();

	lcd.print(String(displayHour) + ":" + String(displayMin) + "AM " + String(months[month]) + " " + String(displayDay));
	lcd.setCursor(0, 1);
	lcd.print(String(year)+" <back> <ok>");
	lcd.setCursor(cursorX, cursorY);
}
	void captureDateTime(){
		hour = rtc.getTime().hour;
		minute = rtc.getTime().min;
		sec = rtc.getTime().sec;
		day = rtc.getTime().date;
		month = rtc.getTime().mon;
		year = rtc.getTime().year;
		captureDateTimeDisplays();
	}
		void captureDateTimeDisplays(){
			suffix = (day == 1 || day == 21 || day == 31) ? "st" : (day == 2 || day == 22) ? "nd" : (day == 3 || day == 23) ? "rd" : "th";
			displayHour = (hour < 10) ? "0" + String(hour) : String(hour);
			displayMin = (minute < 10) ? "0" + String(minute) : String(minute);
			displayDay = (day < 10) ? "0" + String(day) + suffix : String(day) + suffix;
		}

void buildCrop(){
	const int bufferSize = 64;
	String channelName;
	String sessionName;
	File channelSettingsFile;
	File sessionSettingsFile;
	int defaultChannelSize = 10;
	int defaultSessionSize = 3;

	//Parse core file object
	JsonObject& core = getCoreData();
	core["crop"] = cropName;

	//Save change back out to core.
	setCoreData(core);

	lcd.clear();
	lcd.home();
	lcd.print("Building Crop...");
	SD.mkdir(cropName + "/SysConf");
	SD.mkdir(cropName + "/SysConf/DateTime");
	SD.mkdir(cropName + "/SysConf/ChNum");
	SD.mkdir(cropName + "/SysConf/PPM");
	SD.mkdir(cropName + "/SysConf/Open");
	SD.mkdir(cropName + "/SysConf/New");
	SD.mkdir(cropName + "/SysConf/Delete");
	SD.mkdir(cropName + "/Channels");

	//Build Crop Settings File
	StaticJsonBuffer<bufferSize> cropObjBuffer;
	JsonObject& cropSettings = cropObjBuffer.createObject();
	cropSettings["minPPM"] = 1200;
	cropSettings["maxPPM"] = 1400;
	cropSettings["totalChannels"] = 10;
	makeNewFile(cropName + "/crop.dro", cropSettings);

	//Build Channels and their sub sessions
	lcd.setCursor(0, 1);
	for (int i = 0; i < defaultChannelSize; i++){
		channelName = cropName + "/Channels/SysCh" + String(i + 1);
		SD.mkdir(channelName);
		SD.mkdir(channelName + "/ChConf");
		SD.mkdir(channelName + "/ChConf/ChDoses");
		SD.mkdir(channelName + "/ChConf/ChSize");
		SD.mkdir(channelName + "/ChConf/ChCalib");

		//Build Channels Settings File
		StaticJsonBuffer<bufferSize> channelObjBuffer;
		JsonObject& channelSettings = channelObjBuffer.createObject();
		channelSettings["id"] = i;
		channelSettings["size"] = 80;
		channelSettings["sessionsTotal"] = 4;
		channelSettings["calibration"] = 0;

		makeNewFile(channelName + "/channel.dro", channelSettings);

		for (int j = 0; j < defaultSessionSize; j++){
			sessionName = channelName + "/Sessions/ChSes" + String(j + 1);
			SD.mkdir(sessionName);
			SD.mkdir(sessionName + "/Amt");
			SD.mkdir(sessionName + "/Str");
			SD.mkdir(sessionName + "/Dly");
			SD.mkdir(sessionName + "/Rpt");

			//Build Session's settings file
			StaticJsonBuffer<bufferSize> sessionObjBuffer;
			char sessionBuffer[bufferSize];
			JsonObject& sessionSettings = sessionObjBuffer.createObject();

			sessionSettings["id"] = j;
			sessionSettings["channel"] = i;
			sessionSettings["ammount"] = sessionSettings["start"] = sessionSettings["delay"] = sessionSettings["repeatIncrements"] = 0;
			sessionSettings["repeatCount"] = 1; //-2 never, -1 forever, 0 or greater is a fixed number 
			makeNewFile(sessionName + "/session.dro", sessionSettings);
		}
		
		lcd.print("*");
		Serial.flush();
	}
	screenName = "";
	lcd.noBlink();
	lcd.clear();
	lcd.print("LOADING CROP");
	delay(1000);
	lcd.clear();
	File root = SD.open("/" + cropName);
	getDirectoryMenus(root);
	root.close();
	openHomeScreen();
}