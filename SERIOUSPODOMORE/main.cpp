

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <SFML/Audio.hpp> // For alarm sound
#include <iostream>
#include <vector>
#include <string>
#include <memory>       // For smart pointers (unique_ptr)
#include <list>         // Used in NoteManager
#include <map>          // Used in CalendarWidget for events
#include <tuple>        // Used in CalendarWidget event keys
#include <chrono>       // For time, stopwatch
#include <ctime>        // For time formatting
#include <iomanip>      // For formatting time
#include <sstream>      // For string streams
#include <functional>   // For std::function (callbacks)
#include <algorithm>    // For std::remove_if, std::find_if, std::replace, std::find
#include <random>       // For mock weather
#include <cmath>        // For std::round, std::floor
#include <fstream>      // For file I/O (Notes)

using namespace std;
using namespace sf;

// --- Forward Declarations ---
class Widget;
class Button;
class App;

// --- Global Resources ---
Font globalFont;
const float WINDOW_WIDTH = 800.0f;
const float WINDOW_HEIGHT = 600.0f;

// --- Enhanced Color Palette ---
namespace AppColors {
    // Updated Background Colors for a more "colorful" feel
    const Color BackgroundDark = Color(30, 10, 50);    // Deep Dark Purple (Window Clear Color if no image)
    const Color NavBackground = Color(70, 20, 80, 200);     // Medium Purple/Magenta (Navigation Area - slightly transparent)
    const Color WidgetBackground = Color(50, 40, 90, 180);  // Rich Indigo/Blue (Main Widget Area - slightly transparent)
    const Color StatusBackground = Color(60, 15, 70, 200);  // Darker Purple/Magenta (Status Bar - slightly transparent)

    const Color NavActiveBG = Color(110, 70, 150);     // Brighter Purple for active Nav
    const Color NavActiveText = Color::White;
    const Color NavInactiveText = Color(200, 200, 220); // Light Lavender Gray

    const Color ButtonDefault = Color(100, 60, 130);   // Vibrant Purple Button
    const Color ButtonHover = Color(130, 90, 160);   // Lighter Vibrant Purple
    const Color ButtonClick = Color(80, 40, 110);    // Darker Vibrant Purple
    const Color ButtonText = Color::White;
    const Color ButtonDisabledBG = Color(90, 90, 110, 150);
    const Color ButtonDisabledText = Color(180, 180, 190, 150);

    const Color TextDefault = Color(230, 230, 250);    // Lavender White
    const Color TextHeader = Color(255, 215, 0);     // Gold for Headers
    const Color TextAccent1 = Color(60, 179, 113);   // Medium Sea Green
    const Color TextAccent2 = Color(255, 105, 180);  // Hot Pink
    const Color TextWarning = Color(255, 165, 0);    // Orange
    const Color TextPrompt = Color(173, 216, 230);   // Light Blue

    const Color InputBoxBG = Color(60, 60, 100);
    const Color InputBoxOutline = Color(173, 216, 230);
    const Color InputBoxText = Color::White;
    const Color InputBoxCursor = Color(200, 200, 220);

    const Color CalendarTodayBG = Color(34, 139, 34, 200);
    const Color CalendarEventBG = Color(106, 90, 205, 200);
    const Color CalendarDayText = Color::White;
    const Color CalendarEventDayText = Color(255, 223, 186);

    const Color AlarmRingingFlash1 = Color(255, 0, 0);    // Bright Red
    const Color AlarmRingingFlash2 = Color(255, 140, 0);   // Dark Orange
}


// --- Utility Functions ---
// Helper to format time (HH:MM:SS)
string formatTime(int hours, int minutes, int seconds) {
    ostringstream oss;
    oss << setw(2) << setfill('0') << hours << ":"
        << setw(2) << setfill('0') << minutes << ":"
        << setw(2) << setfill('0') << seconds;
    return oss.str();
}

// Helper to format time (HH:MM AM/PM)
string formatTime12Hour(int hours, int minutes) {
    ostringstream oss;
    int displayHour = hours % 12;
    if (displayHour == 0) displayHour = 12;
    oss << setw(2) << setfill('0') << displayHour << ":"
        << setw(2) << setfill('0') << minutes
        << (hours < 12 || hours == 24 ? " AM" : " PM"); // Adjusted for 24 being 12 AM
    return oss.str();
}

// Get the number of days in a given month and year
int daysInMonth(int month, int year) {
    if (month == 2) {
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return isLeap ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    } else {
        return 31;
    }
}

// Get the day of the week (0=Sun, 1=Mon, ..., 6=Sat) for the 1st of the month
int getFirstDayOfMonth(int month, int year) {
    tm time_in = { 0, 0, 0,
                        1, month - 1, year - 1900 };
    time_t time_temp = mktime(&time_in);
    const tm * time_out = localtime(&time_temp);
    if (!time_out) {
         cerr << "Error: Could not determine first day of month for " << month << "/" << year << endl;
        return 0;
    }
    return time_out->tm_wday;
}

// --- Button Class ---
class Button {
public:
    Button(const string& text, const Vector2f& position, const Vector2f& size, const Font& font, unsigned int charSize = 20)
        : m_shape(size), m_text(text, font, charSize), m_isActive(true), m_isHovered(false), m_isClicked(false)
    {
        m_shape.setPosition(position);
        m_shape.setFillColor(AppColors::ButtonDefault);
        m_shape.setOutlineThickness(1);
        m_shape.setOutlineColor(AppColors::InputBoxOutline);

        m_text.setFillColor(AppColors::ButtonText);
        centerText();

        m_defaultColor = AppColors::ButtonDefault;
        m_hoverColor = AppColors::ButtonHover;
        m_clickColor = AppColors::ButtonClick;
        m_disabledColor = AppColors::ButtonDisabledBG;
        m_disabledTextColor = AppColors::ButtonDisabledText;
    }

    void handleEvent(const Event& event, const RenderWindow& window) {
        if (!m_isActive) {
            m_isHovered = false;
            m_isClicked = false;
            return;
        }

        Vector2i mousePos = Mouse::getPosition(window);
        Vector2f worldPos = window.mapPixelToCoords(mousePos);
        m_isHovered = m_shape.getGlobalBounds().contains(worldPos);
        m_isClicked = false;

        if (m_isHovered && event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
            m_isClicked = true;
        }
    }

    void update() {
        if (!m_isActive) {
            m_shape.setFillColor(m_disabledColor);
            m_text.setFillColor(m_disabledTextColor);
        } else if (m_isClicked) {
            m_shape.setFillColor(m_clickColor);
            m_text.setFillColor(AppColors::ButtonText);
        } else if (m_isHovered) {
            m_shape.setFillColor(m_hoverColor);
            m_text.setFillColor(AppColors::ButtonText);
        } else {
            m_shape.setFillColor(m_defaultColor);
            m_text.setFillColor(AppColors::ButtonText);
        }
    }

    void draw(RenderTarget& target) const {
        target.draw(m_shape);
        target.draw(m_text);
    }

    bool isClicked() const {
        return m_isClicked && m_isActive;
    }

    FloatRect getGlobalBounds() const {
        return m_shape.getGlobalBounds();
    }

    void setText(const string& text) {
         m_text.setString(text);
         centerText();
    }

    string getText() const {
        return m_text.getString();
    }

    void setPosition(const Vector2f& pos) {
        m_shape.setPosition(pos);
        centerText();
    }

    void setActive(bool active) {
        m_isActive = active;
        if (!m_isActive) {
             m_shape.setFillColor(m_disabledColor);
             m_text.setFillColor(m_disabledTextColor);
             m_isHovered = false;
             m_isClicked = false;
        } else {
            update();
        }
    }

    bool isActive() const {
        return m_isActive;
    }

private:
    void centerText() {
        FloatRect textBounds = m_text.getLocalBounds();
        m_text.setOrigin(std::floor(textBounds.left + textBounds.width / 2.0f), std::floor(textBounds.top + textBounds.height / 2.0f));
        m_text.setPosition(std::floor(m_shape.getPosition().x + m_shape.getSize().x / 2.0f), std::floor(m_shape.getPosition().y + m_shape.getSize().y / 2.0f));
    }

    RectangleShape m_shape;
    Text m_text;
    bool m_isActive;
    bool m_isHovered;
    bool m_isClicked;

    Color m_defaultColor;
    Color m_hoverColor;
    Color m_clickColor;
    Color m_disabledColor;
    Color m_disabledTextColor;
};

// --- Base Widget Class (Abstract) ---
class Widget {
public:
    virtual ~Widget() = default;
    virtual void handleEvent(const Event& event, const RenderWindow& window) = 0;
    virtual void update(float dt) = 0;
    virtual void draw(RenderTarget& target) const = 0;
    virtual void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) {
        m_font = &font;
        m_areaPosition = areaPosition;
        m_areaSize = areaSize;
    };
    virtual void activate() { m_isActive = true; m_isAcceptingTextInput = false; };
    virtual void deactivate() { m_isActive = false; m_isAcceptingTextInput = false; };

    bool isActive() const { return m_isActive; }
    virtual bool isAcceptingTextInput() const { return m_isAcceptingTextInput; }

protected:
    bool m_isActive = false;
    bool m_isAcceptingTextInput = false;
    const Font* m_font = nullptr;
    Vector2f m_areaPosition;
    Vector2f m_areaSize;
};

// --- Clock Widget ---
class ClockWidget : public Widget {
public:
    ClockWidget() : m_use24HourFormat(true) {}

    void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) override {
        Widget::setupUI(font, areaPosition, areaSize);
        m_timeText.setFont(*m_font);
        m_timeText.setCharacterSize(52);
        m_timeText.setFillColor(AppColors::TextHeader);

        m_dateText.setFont(*m_font);
        m_dateText.setCharacterSize(28);
        m_dateText.setFillColor(AppColors::TextDefault);

        m_dayText.setFont(*m_font);
        m_dayText.setCharacterSize(28);
        m_dayText.setFillColor(AppColors::TextDefault);

        updateTextPositions();
        update(0.0f);
    }

     void updateTextPositions() {
         float totalHeight = m_timeText.getLocalBounds().height + m_dateText.getLocalBounds().height + m_dayText.getLocalBounds().height + 40;
         float currentY = m_areaPosition.y + (m_areaSize.y - totalHeight) / 2.0f;

         FloatRect textBounds = m_timeText.getLocalBounds();
         m_timeText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top);
         m_timeText.setPosition(m_areaPosition.x + m_areaSize.x / 2.0f, currentY);
         currentY += textBounds.height + 15;

         textBounds = m_dateText.getLocalBounds();
         m_dateText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top);
         m_dateText.setPosition(m_areaPosition.x + m_areaSize.x / 2.0f, currentY);
         currentY += textBounds.height + 15;

         textBounds = m_dayText.getLocalBounds();
         m_dayText.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top);
         m_dayText.setPosition(m_areaPosition.x + m_areaSize.x / 2.0f, currentY);
    }

    void handleEvent(const Event& event, const RenderWindow& window) override {
        if (m_isActive && event.type == Event::KeyPressed) {
            if (event.key.code == Keyboard::F) {
                m_use24HourFormat = !m_use24HourFormat;
                update(0.0f);
            }
        }
    }

    void update(float dt) override {
        auto now = chrono::system_clock::now();
        auto now_c = chrono::system_clock::to_time_t(now);
        tm now_tm = *localtime(&now_c);

        ostringstream timeStream;
        timeStream << put_time(&now_tm, m_use24HourFormat ? "%H:%M:%S" : "%I:%M:%S %p");
        m_timeText.setString(timeStream.str());

        ostringstream dateStream;
        dateStream << put_time(&now_tm, "%Y-%m-%d");
        m_dateText.setString(dateStream.str());

        ostringstream dayStream;
        dayStream << put_time(&now_tm, "%A");
        m_dayText.setString(dayStream.str());

        updateTextPositions();
    }

    void draw(RenderTarget& target) const override {
        target.draw(m_timeText);
        target.draw(m_dateText);
        target.draw(m_dayText);
    }

private:
    Text m_timeText;
    Text m_dateText;
    Text m_dayText;
    bool m_use24HourFormat;
};

// --- Weather Widget  ---
class WeatherWidget : public Widget {
public:
    WeatherWidget() : m_temperature(0.0), m_humidity(0.0), m_condition("Clear") {}

     void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) override {
        Widget::setupUI(font, areaPosition, areaSize);
        m_locationText.setFont(*m_font);
        m_locationText.setCharacterSize(30);
        m_locationText.setFillColor(AppColors::TextHeader);
        m_locationText.setString("Weather: Topi, KP");
        m_locationText.setPosition(areaPosition.x + 20, areaPosition.y + 30);

        m_tempText.setFont(*m_font);
        m_tempText.setCharacterSize(40);
        m_tempText.setFillColor(AppColors::TextAccent1);
        m_tempText.setPosition(areaPosition.x + 20, areaPosition.y + 80);

        m_humidityText.setFont(*m_font);
        m_humidityText.setCharacterSize(26);
        m_humidityText.setFillColor(AppColors::TextDefault);
        m_humidityText.setPosition(areaPosition.x + 20, areaPosition.y + 140);

        m_conditionText.setFont(*m_font);
        m_conditionText.setCharacterSize(26);
        m_conditionText.setFillColor(AppColors::TextDefault);
        m_conditionText.setPosition(areaPosition.x + 20, areaPosition.y + 180);

        updateMockData();
    }

    void handleEvent(const Event& event, const RenderWindow& window) override {
         if (m_isActive && event.type == Event::KeyPressed) {
            if (event.key.code == Keyboard::U) {
                updateMockData();
            }
        }
    }

    void update(float dt) override {
        m_tempText.setString("Temp: " + to_string(static_cast<int>(std::round(m_temperature))) + " C");
        m_humidityText.setString("Humidity: " + to_string(static_cast<int>(std::round(m_humidity))) + "%");
        m_conditionText.setString("Condition: " + m_condition);
    }

    void draw(RenderTarget& target) const override {
        target.draw(m_locationText);
        target.draw(m_tempText);
        target.draw(m_humidityText);
        target.draw(m_conditionText);
    }

private:
    void updateMockData() {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_real_distribution<> tempDist(15.0, 35.0);
        uniform_real_distribution<> humDist(20.0, 70.0);
        uniform_int_distribution<> condDist(0, 4);

        m_temperature = tempDist(gen);
        m_humidity = humDist(gen);

        switch (condDist(gen)) {
            case 0: m_condition = "Clear"; break;
            case 1: m_condition = "Partly Cloudy"; break;
            case 2: m_condition = "Cloudy"; break;
            case 3: m_condition = "Hazy"; break;
            case 4: m_condition = "Sunny"; break;
        }
         update(0.0f);
    }

    Text m_locationText;
    Text m_tempText;
    Text m_humidityText;
    Text m_conditionText;
    double m_temperature;
    double m_humidity;
    string m_condition;
};

class NoteManager {
public:
    struct Note {
        int id = 0;
        string heading;
        string content;
    };

    NoteManager(const string& filename = "notes_v2.txt") : m_filename(filename) {
        loadNotes();
    }

    ~NoteManager() {
        saveNotes();
    }

    void addNote(const string& heading, const string& content) {
        if (!heading.empty() || !content.empty()) {
            m_notes.push_back({m_nextId++, heading, content});
            saveNotes();
        }
    }

    void removeNote(int id) {
        auto it = std::remove_if(m_notes.begin(), m_notes.end(),
                                 [id](const Note& note) { return note.id == id; });
        if (it != m_notes.end()) {
            m_notes.erase(it, m_notes.end());
            saveNotes();
        }
    }

    bool editNote(int id, const string& newHeading, const string& newContent) {
        auto it = std::find_if(m_notes.begin(), m_notes.end(),
                               [id](const Note& note) { return note.id == id; });
        if (it != m_notes.end()) {
            if (!newHeading.empty() || !newContent.empty()) {
                it->heading = newHeading;
                it->content = newContent;
                saveNotes();
                return true;
            } else {
                cerr << "Warning: Edit resulted in empty note (ID: " << id << "). Not saved." << endl;
                return false;
            }
        }
        return false;
    }

    const Note* getNoteById(int id) const {
        auto it = std::find_if(m_notes.begin(), m_notes.end(),
                               [id](const Note& note) { return note.id == id; });
        if (it != m_notes.end()) {
            return &(*it);
        }
        return nullptr;
    }

    const vector<Note>& getNotes() const {
        return m_notes;
    }

private:
    void loadNotes() {
        m_notes.clear();
        m_nextId = 1;
        ifstream inFile(m_filename);
        int maxId = 0;

        if (!inFile) {
            cerr << "Warning: Notes file not found: " << m_filename << ". A new file will be created on save." << endl;
            return;
        }

        string line;
        bool isFirstLine = true;

        while (getline(inFile, line)) {
            // Strip Windows \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Remove UTF-8 BOM if it's the first line
            if (isFirstLine && line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;

            // cerr << "DEBUG: Raw line = [" << line << "]" << endl; // Optional debug

            size_t idSeparatorPos = line.find(':');
            if (idSeparatorPos == string::npos) {
                cerr << "Warning: Skipping invalid note line (no ID separator): " << line << endl;
                continue;
            }

            size_t headingSeparatorPos = line.find('|', idSeparatorPos + 1);
            string idStr = line.substr(0, idSeparatorPos);
            string heading, content;

            if (headingSeparatorPos == string::npos) {
                heading = line.substr(idSeparatorPos + 1);
                content = "";
                cerr << "Warning: Note line missing content separator '|': " << line
                          << ". Assuming empty content for heading: " << heading << endl;
            } else {
                heading = line.substr(idSeparatorPos + 1, headingSeparatorPos - (idSeparatorPos + 1));
                content = line.substr(headingSeparatorPos + 1);
            }

            try {
                int id = stoi(idStr);
                m_notes.push_back({id, heading, content});
                if (id > maxId) {
                    maxId = id;
                }
                
            } catch (const invalid_argument&) {
                cerr << "Warning: Skipping invalid note line (ID parse error for '" << idStr << "'): " << line << endl;
            } catch (const out_of_range&) {
                cerr << "Warning: Skipping invalid note line (ID out of range for '" << idStr << "'): " << line << endl;
            }
        }

        inFile.close();

        if (!m_notes.empty()) {
            m_nextId = maxId + 1;
        } else {
            m_nextId = 1;
        }
    }

    void saveNotes() {
        ofstream outFile(m_filename);
        if (!outFile) {
            cerr << "Error: Could not open notes file for writing: " << m_filename << endl;
            return;
        }
        for (const auto& note : m_notes) {
            outFile << note.id << ":" << note.heading << "|" << note.content << "\n";
        }
        outFile.close();
    }

    string m_filename;
    vector<Note> m_notes; // changed from std::list to std::vector
    int m_nextId;
};

// --- Notes Widget (with Headings, Edit, Delete, Confirmation) ---
class NotesWidget : public Widget {
// Friend class declaration for App to access m_inputState (for specific key handling)
friend class App;
private:
    enum class InputState { None, EnteringHeading, EnteringContent, EditingHeading, EditingContent, ConfirmDelete };
public:
    NotesWidget(NoteManager& manager)
        : m_noteManager(manager), m_inputState(InputState::None), m_scrollOffset(0.0f), m_noteToSelect(-1), m_editingNoteId(-1) {}

     void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) override {
        Widget::setupUI(font, areaPosition, areaSize);
        m_titleText.setFont(*m_font);
        m_titleText.setString("Notes:");
        m_titleText.setCharacterSize(32); // Increased font size
        m_titleText.setFillColor(AppColors::TextHeader);
        m_titleText.setPosition(areaPosition.x + 15, areaPosition.y + 10);

        float buttonWidth = 120;
        float buttonHeight = 35;
        float buttonSpacing = 15;
        float buttonsTotalWidth = buttonWidth * 3 + buttonSpacing * 2;
        float buttonsStartX = areaPosition.x + areaSize.x - buttonsTotalWidth - 15;


        m_addNoteButton = make_unique<Button>("Add Note", Vector2f(buttonsStartX, areaPosition.y + 8), Vector2f(buttonWidth, buttonHeight), *m_font);
        m_editNoteButton = make_unique<Button>("Edit Note", Vector2f(buttonsStartX + buttonWidth + buttonSpacing, areaPosition.y + 8), Vector2f(buttonWidth, buttonHeight), *m_font);
        m_deleteNoteButton = make_unique<Button>("Delete Note", Vector2f(buttonsStartX + 2 * (buttonWidth + buttonSpacing), areaPosition.y + 8), Vector2f(buttonWidth, buttonHeight), *m_font);

        m_editNoteButton->setActive(false);
        m_deleteNoteButton->setActive(false);

        m_inputBox.setSize(Vector2f(areaSize.x - 30, 35));
        m_inputBox.setPosition(areaPosition.x + 15, areaPosition.y + areaSize.y - 50);
        m_inputBox.setFillColor(AppColors::InputBoxBG);
        m_inputBox.setOutlineThickness(1);
        m_inputBox.setOutlineColor(AppColors::InputBoxOutline);

        m_inputText.setFont(*m_font);
        m_inputText.setCharacterSize(18);
        m_inputText.setFillColor(AppColors::InputBoxText);
        m_inputText.setPosition(m_inputBox.getPosition().x + 8, m_inputBox.getPosition().y + 8);

        m_inputPrompt.setFont(*m_font);
        m_inputPrompt.setCharacterSize(16);
        m_inputPrompt.setFillColor(AppColors::TextPrompt);
        m_inputPrompt.setPosition(m_inputBox.getPosition().x, m_inputBox.getPosition().y - 22);

        m_notesDisplayView.reset(FloatRect(areaPosition.x, areaPosition.y + 55, areaSize.x, areaSize.y - 115));
        m_notesDisplayView.setViewport(FloatRect(areaPosition.x / WINDOW_WIDTH, (areaPosition.y + 55) / WINDOW_HEIGHT,
                                                   areaSize.x / WINDOW_WIDTH, (areaSize.y - 115) / WINDOW_HEIGHT));

        m_notesDisplayText.setFont(*m_font);
        m_notesDisplayText.setCharacterSize(17);
        m_notesDisplayText.setFillColor(AppColors::TextDefault);
        m_notesDisplayText.setPosition(15, 10);
        m_notesDisplayText.setLineSpacing(1.25f);

        refreshNotesDisplay();
    }

    void activate() override {
        Widget::activate();
        refreshNotesDisplay();
        cancelInput();
    }

     bool isAcceptingTextInput() const override {
        return m_inputState == InputState::EnteringHeading || m_inputState == InputState::EnteringContent ||
               m_inputState == InputState::EditingHeading || m_inputState == InputState::EditingContent;
    }

    void handleEvent(const Event& event, const RenderWindow& window) override {
        if (!m_isActive) return;

        m_addNoteButton->handleEvent(event, window);
        m_editNoteButton->handleEvent(event, window);
        m_deleteNoteButton->handleEvent(event, window);

        if (m_addNoteButton->isClicked() && m_inputState != InputState::ConfirmDelete) {
            m_inputState = InputState::EnteringHeading;
            m_currentHeading = "";
            m_currentContent = "";
            m_editingNoteId = -1;
            m_inputText.setString("|");
            m_inputPrompt.setString("Enter Heading (Press Enter):");
            m_isAcceptingTextInput = true;
            m_noteToSelect = -1;
            m_editNoteButton->setActive(false);
            m_deleteNoteButton->setActive(false);
            m_deleteNoteButton->setText("Delete Note");
            refreshNotesDisplay();
            return;
        }

        if (m_editNoteButton->isClicked() && m_noteToSelect != -1 && m_inputState != InputState::ConfirmDelete) {
            const NoteManager::Note* noteToEdit = m_noteManager.getNoteById(m_noteToSelect);
            if (noteToEdit) {
                m_inputState = InputState::EditingHeading;
                m_currentHeading = noteToEdit->heading;
                m_currentContent = noteToEdit->content;
                m_editingNoteId = noteToEdit->id;
                m_inputText.setString(m_currentHeading + "|");
                m_inputPrompt.setString("Edit Heading (Press Enter):");
                m_isAcceptingTextInput = true;
                m_addNoteButton->setActive(false);
                m_deleteNoteButton->setActive(false);
                m_editNoteButton->setActive(false);
            }
            return;
        }

        if (m_deleteNoteButton->isClicked() && m_noteToSelect != -1) {
            if (m_inputState == InputState::ConfirmDelete) {
                m_noteManager.removeNote(m_noteToSelect);
                m_noteToSelect = -1;
                cancelInput();
            } else {
                m_inputState = InputState::ConfirmDelete;
                m_inputPrompt.setString("Confirm delete Note ID " + to_string(m_noteToSelect) + "? (Enter or click Delete)");
                m_inputPrompt.setFillColor(AppColors::TextWarning); // Warning color for prompt
                m_deleteNoteButton->setText("CONFIRM");
                m_addNoteButton->setActive(false);
                m_editNoteButton->setActive(false);
                m_isAcceptingTextInput = false;
            }
            refreshNotesDisplay();
            return;
        }

        if (m_inputState != InputState::None) {
            if (event.type == Event::TextEntered) {
                if (isAcceptingTextInput()) {
                     handleTextInput(event.text.unicode);
                } else if (m_inputState == InputState::ConfirmDelete && (event.text.unicode == '\r' || event.text.unicode == '\n')) {
                    m_noteManager.removeNote(m_noteToSelect);
                    m_noteToSelect = -1;
                    cancelInput();
                }
                return;
            }

            if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
                 cancelInput();
                 return;
            }

            if (event.type == Event::MouseButtonPressed) {
                 Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
                 if (!m_inputBox.getGlobalBounds().contains(mousePos) &&
                     !m_addNoteButton->getGlobalBounds().contains(mousePos) &&
                     !m_editNoteButton->getGlobalBounds().contains(mousePos) &&
                     !m_deleteNoteButton->getGlobalBounds().contains(mousePos))
                 {
                    if (m_inputState == InputState::ConfirmDelete || isAcceptingTextInput()){
                        cancelInput();
                    }
                    return;
                 }
             }
        }
        else {
             if (event.type == Event::MouseWheelScrolled) {
                 Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
                 FloatRect notesAreaRect(m_areaPosition.x, m_areaPosition.y + 55, m_areaSize.x, m_areaSize.y - 115);
                 if (notesAreaRect.contains(mousePos)) {
                     if (event.mouseWheelScroll.wheel == Mouse::VerticalWheel) {
                         m_scrollOffset -= event.mouseWheelScroll.delta * 25;
                         float contentHeight = m_notesDisplayText.getGlobalBounds().height;
                         float viewHeight = m_notesDisplayView.getSize().y;
                         float maxScroll = std::max(0.0f, contentHeight - viewHeight + 20);
                         m_scrollOffset = std::max(0.0f, std::min(m_scrollOffset, maxScroll));
                         m_notesDisplayText.setPosition(15, 10 - m_scrollOffset);
                     }
                 }
                 return;
             }

             if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
                Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
                FloatRect notesAreaRect(m_areaPosition.x, m_areaPosition.y + 55, m_areaSize.x, m_areaSize.y - 115);

                if (notesAreaRect.contains(mousePos)) {
                    Vector2f textRelativePos = window.mapPixelToCoords(Mouse::getPosition(window), m_notesDisplayView);
                    textRelativePos.x -= m_notesDisplayText.getPosition().x;
                    textRelativePos.y -= m_notesDisplayText.getPosition().y;

                    int clickedNoteId = findClickedNoteId(textRelativePos);
                    if (clickedNoteId != -1) {
                        if (m_noteToSelect == clickedNoteId) {
                            // Double click action: Start editing
                            const NoteManager::Note* noteToEdit = m_noteManager.getNoteById(m_noteToSelect);
                            if (noteToEdit) {
                                m_inputState = InputState::EditingHeading;
                                m_currentHeading = noteToEdit->heading;
                                m_currentContent = noteToEdit->content;
                                m_editingNoteId = noteToEdit->id;
                                m_inputText.setString(m_currentHeading + "|");
                                m_inputPrompt.setString("Edit Heading (Press Enter):");
                                m_isAcceptingTextInput = true;
                                m_addNoteButton->setActive(false);
                                m_deleteNoteButton->setActive(false);
                                m_editNoteButton->setActive(false);
                            }
                        } else {
                            m_noteToSelect = clickedNoteId;
                            m_editNoteButton->setActive(true);
                            m_deleteNoteButton->setActive(true);
                        }
                    } else {
                        m_noteToSelect = -1;
                        m_editNoteButton->setActive(false);
                        m_deleteNoteButton->setActive(false);
                    }
                     refreshNotesDisplay();
                    return;
                } else { // Clicked outside notes display area but still in widget
                     m_noteToSelect = -1;
                     m_editNoteButton->setActive(false);
                     m_deleteNoteButton->setActive(false);
                     refreshNotesDisplay();
                }
             }
        }
    }

    void update(float dt) override {
         m_addNoteButton->update();
         m_editNoteButton->update();
         m_deleteNoteButton->update();

        static Clock cursorClock;
        if (isAcceptingTextInput()) {
            if (cursorClock.getElapsedTime().asSeconds() > 0.5f) {
                 string currentDisplay = m_inputText.getString();
                 string baseString = (m_inputState == InputState::EnteringHeading || m_inputState == InputState::EditingHeading) ? m_currentHeading : m_currentContent;
                if (!currentDisplay.empty() && currentDisplay.length() > baseString.length() && currentDisplay.back() == '|') {
                    m_inputText.setString(baseString);
                } else {
                    m_inputText.setString(baseString + "|");
                }
                 cursorClock.restart();
             }
        } else {
             if (!m_inputText.getString().isEmpty() && m_inputState != InputState::ConfirmDelete) {
                 m_inputText.setString("");
             }
        }
    }

    void draw(RenderTarget& target) const override {
        target.draw(m_titleText);
        m_addNoteButton->draw(target);
        m_editNoteButton->draw(target);
        m_deleteNoteButton->draw(target);

        View originalView = target.getView();
        target.setView(m_notesDisplayView);
        target.draw(m_notesDisplayText);
        target.setView(originalView);

        if (m_inputState != InputState::None) {
            target.draw(m_inputPrompt);
            if (m_inputState != InputState::ConfirmDelete) {
                 target.draw(m_inputBox);
                 target.draw(m_inputText);
            }
        }
    }

    void refreshNotesDisplay() {
        stringstream ss;
        const auto& notes = m_noteManager.getNotes();
        m_lineInfo.clear();

        if (notes.empty()) {
            ss << "No notes added yet. Click 'Add Note'.";
            m_lineInfo.push_back({0, -1, false});
        } else {
            int lineNum = 0;
            for (const auto& note : notes) {
                string prefix = (note.id == m_noteToSelect) ? "* " : "  ";
                string headingStr = prefix + to_string(note.id) + ". [" + note.heading + "]";
                ss << headingStr << "\n";
                m_lineInfo.push_back({lineNum, note.id, true});
                lineNum++;

                string contentStr = "    " + note.content;
                float avgCharWidth = (m_notesDisplayText.getFont() ? m_notesDisplayText.getFont()->getGlyph('M', m_notesDisplayText.getCharacterSize(), false).advance : 8.0f) * 0.8f;
                size_t maxLineLength = static_cast<size_t>((m_notesDisplayView.getSize().x - 30) / avgCharWidth) ;

                size_t currentPos = 0;
                while (currentPos < contentStr.length()) {
                    size_t lengthToTake = std::min(maxLineLength, contentStr.length() - currentPos);
                    size_t lineEnd = currentPos + lengthToTake;

                    // Look for the last space within the range
                    if (lineEnd < contentStr.length()) {
                        size_t lastSpace = contentStr.rfind(' ', lineEnd);
                        if (lastSpace != string::npos && lastSpace > currentPos && lastSpace <= lineEnd) {
                            lineEnd = lastSpace;
                        }
                    }

                    string lineSegment = contentStr.substr(currentPos, lineEnd - currentPos);
                    ss << lineSegment << "\n";
                    m_lineInfo.push_back({lineNum, note.id, false});
                    lineNum++;

                    currentPos = lineEnd;
                    if (currentPos < contentStr.length() && contentStr[currentPos] == ' ') {
                        currentPos++; // Skip the space
                    }
                }

                 if (!note.content.empty() || !note.heading.empty()) {
                    ss << "\n";
                    m_lineInfo.push_back({lineNum, -1, false});
                    lineNum++;
                 }
            }
        }
        m_notesDisplayText.setString(ss.str());

        float contentHeight = m_notesDisplayText.getGlobalBounds().height;
        float viewHeight = m_notesDisplayView.getSize().y;
        float maxScroll = std::max(0.0f, contentHeight - viewHeight + 20);
        m_scrollOffset = std::max(0.0f, std::min(m_scrollOffset, maxScroll));
        m_notesDisplayText.setPosition(15, 10 - m_scrollOffset);
    }


private:
    void handleTextInput(Uint32 unicode) {
        string& currentBuffer = (m_inputState == InputState::EnteringHeading || m_inputState == InputState::EditingHeading) ? m_currentHeading : m_currentContent;

        if (unicode == '\b') {
            if (!currentBuffer.empty()) {
                currentBuffer.pop_back();
             }
        } else if (unicode == '\r' || unicode == '\n') {
            if (m_inputState == InputState::EnteringHeading) {
                m_inputState = InputState::EnteringContent;
                m_inputPrompt.setString("Enter Content (Press Enter to Save):");
                m_currentContent = "";
                m_inputText.setString("|");
            } else if (m_inputState == InputState::EnteringContent) {
                 if (!m_currentHeading.empty() || !m_currentContent.empty()) {
                     m_noteManager.addNote(m_currentHeading, m_currentContent);
                 }
                cancelInput();
            } else if (m_inputState == InputState::EditingHeading) {
                 m_inputState = InputState::EditingContent;
                 m_inputPrompt.setString("Edit Content (Press Enter to Save):");
                 m_inputText.setString(m_currentContent + "|");
            } else if (m_inputState == InputState::EditingContent) {
                 if (m_editingNoteId != -1) {
                     m_noteManager.editNote(m_editingNoteId, m_currentHeading, m_currentContent);
                 }
                 cancelInput();
            }
        } else if (unicode >= 32 && unicode < 128 && unicode != '\t') {
            if (currentBuffer.length() < 2048) {
                 currentBuffer += static_cast<char>(unicode);
            }
        }
        string baseString = (m_inputState == InputState::EnteringHeading || m_inputState == InputState::EditingHeading) ? m_currentHeading : m_currentContent;
        m_inputText.setString(baseString + "|");
    }

    void cancelInput() {
        m_inputState = InputState::None;
        m_currentHeading = "";
        m_currentContent = "";
        m_inputText.setString("");
        m_inputPrompt.setString("");
        m_inputPrompt.setFillColor(AppColors::TextPrompt); // Reset prompt color
        m_isAcceptingTextInput = false;
        m_editingNoteId = -1;

        m_deleteNoteButton->setText("Delete Note");

        refreshNotesDisplay();

        m_addNoteButton->setActive(true);
        m_editNoteButton->setActive(m_noteToSelect != -1);
        m_deleteNoteButton->setActive(m_noteToSelect != -1);
    }

    struct LineNoteInfo {
        int lineNumber;
        int noteId;
        bool isHeading;
    };
    vector<LineNoteInfo> m_lineInfo;

    int findClickedNoteId(const Vector2f& textRelativeClickPos) {
        float lineHeight = m_notesDisplayText.getCharacterSize() * m_notesDisplayText.getLineSpacing();
        if (lineHeight <= 0) return -1;

        int clickedLine = static_cast<int>(textRelativeClickPos.y / lineHeight);
        for(const auto& info : m_lineInfo) {
            if (info.lineNumber == clickedLine) {
                return info.isHeading ? info.noteId : -1;
            }
        }
        return -1;
    }

    NoteManager& m_noteManager;
    Text m_titleText;
    Text m_notesDisplayText;
    unique_ptr<Button> m_addNoteButton;
    unique_ptr<Button> m_editNoteButton;
    unique_ptr<Button> m_deleteNoteButton;
    View m_notesDisplayView;

    RectangleShape m_inputBox;
    Text m_inputText;
    Text m_inputPrompt;
    string m_currentHeading;
    string m_currentContent;
    InputState m_inputState; 
    float m_scrollOffset;
    int m_noteToSelect;
    int m_editingNoteId;
};

// --- Calendar Widget (Functional Implementation) ---
class CalendarWidget : public Widget {
public:
    CalendarWidget() : m_currentYear(0), m_currentMonth(0) {
        auto now = chrono::system_clock::now();
        auto now_c = chrono::system_clock::to_time_t(now);
        tm now_tm = *localtime(&now_c);
        m_currentYear = now_tm.tm_year + 1900;
        m_currentMonth = now_tm.tm_mon + 1;

        m_events[{3, 23}] = "Pakistan Day";
        m_events[{5, 1}] = "Labour Day";
        m_events[{8, 14}] = "Independence Day";
        m_events[{9, 6}] = "Defence Day";
        m_events[{11, 9}] = "Iqbal Day";
        m_events[{12, 25}] = "Quaid-e-Azam Day / Christmas";
        m_events[{5, 23}] = "Miss Nazias Birthday!";
        m_events[{11, 28}] = "Momin's Birthday!";
        m_events[{12, 7}] = "Fahad's Birthday!";
        m_events[{11, 21}] = "Munawar's Birthday!";
        m_events[{12, 5}] = "Ahmad's Birthday!";
        //m_events[{12, 12}] = "Momin's Birthday!";

    }

     void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) override {
         Widget::setupUI(font, areaPosition, areaSize);
         m_monthYearText.setFont(*m_font);
         m_monthYearText.setCharacterSize(30);
         m_monthYearText.setFillColor(AppColors::TextHeader);

         float buttonSize = 35.f;
         m_prevMonthButton = make_unique<Button>("<", Vector2f(areaPosition.x + 15, areaPosition.y + 15), Vector2f(buttonSize, buttonSize), *m_font, 22);
         m_nextMonthButton = make_unique<Button>(">", Vector2f(areaPosition.x + areaSize.x - buttonSize - 15, areaPosition.y + 15), Vector2f(buttonSize, buttonSize), *m_font, 22);

         const char* dayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
         m_dayHeaderTexts.resize(7);
         float dayCellWidth = areaSize.x / 7.0f;
         float headerY = areaPosition.y + 70;
         for (int i = 0; i < 7; ++i) {
            m_dayHeaderTexts[i].setFont(*m_font);
            m_dayHeaderTexts[i].setString(dayNames[i]);
            m_dayHeaderTexts[i].setCharacterSize(18);
            m_dayHeaderTexts[i].setFillColor(AppColors::TextAccent1);
            FloatRect bounds = m_dayHeaderTexts[i].getLocalBounds();
            m_dayHeaderTexts[i].setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
            m_dayHeaderTexts[i].setPosition(areaPosition.x + dayCellWidth * i + dayCellWidth / 2.0f, headerY);
        }

         m_eventDisplayText.setFont(*m_font);
         m_eventDisplayText.setCharacterSize(20);
         m_eventDisplayText.setFillColor(AppColors::TextAccent2);
         m_eventDisplayText.setPosition(areaPosition.x + 15, areaPosition.y + areaSize.y - 40);

         updateCalendarGrid();
    }

    void handleEvent(const Event& event, const RenderWindow& window) override {
        if (!m_isActive) return;
        m_prevMonthButton->handleEvent(event, window);
        m_nextMonthButton->handleEvent(event, window);

        if (m_prevMonthButton->isClicked()) {
            m_currentMonth--;
            if (m_currentMonth < 1) {
                m_currentMonth = 12;
                m_currentYear--;
            }
            updateCalendarGrid();
        }

        if (m_nextMonthButton->isClicked()) {
            m_currentMonth++;
            if (m_currentMonth > 12) {
                m_currentMonth = 1;
                m_currentYear++;
            }
            updateCalendarGrid();
        }

         if (event.type == Event::MouseButtonPressed && event.mouseButton.button == Mouse::Left) {
             Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
             float gridStartY = m_areaPosition.y + 95;
             float gridEndY = m_eventDisplayText.getPosition().y - 15;
            if (mousePos.y > gridStartY && mousePos.y < gridEndY) {
                 int clickedDay = getDayFromPosition(mousePos);
                 if (clickedDay > 0) {
                     auto eventIt = m_events.find({m_currentMonth, clickedDay});
                     if (eventIt != m_events.end()) {
                        m_eventDisplayText.setString("Event: " + eventIt->second);
                     } else {
                        m_eventDisplayText.setString("No event for this day.");
                     }
                 } else {
                      m_eventDisplayText.setString("");
                 }
             } else {
                  m_eventDisplayText.setString("");
             }
         }
    }

    void update(float dt) override {
        m_prevMonthButton->update();
        m_nextMonthButton->update();
    }

    void draw(RenderTarget& target) const override {
        m_prevMonthButton->draw(target);
        m_nextMonthButton->draw(target);
        target.draw(m_monthYearText);

        for (const auto& header : m_dayHeaderTexts) {
            target.draw(header);
        }
        for (const auto& cell : m_dayCells) {
            target.draw(cell.background);
            target.draw(cell.numberText);
        }
        target.draw(m_eventDisplayText);
    }

private:
    struct DayCell {
        RectangleShape background;
        Text numberText;
        int dayNumber = 0;
        bool isToday = false;
        bool hasEvent = false;
    };

    void updateCalendarGrid() {
        const char* monthNames[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
        m_monthYearText.setString(string(monthNames[m_currentMonth - 1]) + " " + to_string(m_currentYear));
        FloatRect monthBounds = m_monthYearText.getLocalBounds();
        m_monthYearText.setOrigin(monthBounds.left + monthBounds.width / 2.0f, monthBounds.top + monthBounds.height / 2.0f);
        float textX = m_prevMonthButton->getGlobalBounds().left + m_prevMonthButton->getGlobalBounds().width +
                      (m_nextMonthButton->getGlobalBounds().left - (m_prevMonthButton->getGlobalBounds().left + m_prevMonthButton->getGlobalBounds().width)) / 2.0f;
        m_monthYearText.setPosition(textX, m_prevMonthButton->getGlobalBounds().top + m_prevMonthButton->getGlobalBounds().height / 2.0f + 5);

        int days = daysInMonth(m_currentMonth, m_currentYear);
        int firstDay = getFirstDayOfMonth(m_currentMonth, m_currentYear);

        auto now_sys = chrono::system_clock::now();
        auto now_c = chrono::system_clock::to_time_t(now_sys);
        tm today_tm = *localtime(&now_c);
        int todayYear = today_tm.tm_year + 1900;
        int todayMonth = today_tm.tm_mon + 1;
        int todayDay = today_tm.tm_mday;

        m_dayCells.clear();
        m_dayCells.resize(42);

        float dayCellWidth = m_areaSize.x / 7.0f;
        float gridStartY = m_areaPosition.y + 95;
        float gridEndY = m_eventDisplayText.getPosition().y - 15;
        float availableHeight = std::max(100.0f, gridEndY - gridStartY);
        float dayCellHeight = availableHeight / 6.0f;

        int dayCounter = 1;
        for (int i = 0; i < 42; ++i) {
            int row = i / 7;
            int col = i % 7;
            float cellX = m_areaPosition.x + col * dayCellWidth;
            float cellY = gridStartY + row * dayCellHeight;

            m_dayCells[i].background.setSize({dayCellWidth - 2, dayCellHeight - 2});
            m_dayCells[i].background.setPosition(cellX + 1, cellY + 1);
            m_dayCells[i].background.setOutlineThickness(1);
            m_dayCells[i].background.setOutlineColor(AppColors::NavBackground);

            m_dayCells[i].numberText.setFont(*m_font);
            m_dayCells[i].numberText.setCharacterSize(17);

            if (row == 0 && col < firstDay) { //Leaves blank space logic
                m_dayCells[i].background.setFillColor(AppColors::WidgetBackground);
                m_dayCells[i].numberText.setString("");
                m_dayCells[i].dayNumber = 0;
            } else if (dayCounter <= days) {
                m_dayCells[i].dayNumber = dayCounter;
                m_dayCells[i].numberText.setString(to_string(dayCounter));
                m_dayCells[i].isToday = (m_currentYear == todayYear && m_currentMonth == todayMonth && dayCounter == todayDay);
                auto eventIt = m_events.find({m_currentMonth, dayCounter});
                m_dayCells[i].hasEvent = (eventIt != m_events.end());

                if (m_dayCells[i].isToday) { //Checks for todays date and logic for color of events and normal days
                    m_dayCells[i].background.setFillColor(AppColors::CalendarTodayBG);
                    m_dayCells[i].numberText.setFillColor(Color::White);
                    m_dayCells[i].numberText.setStyle(Text::Bold);
                } else if (m_dayCells[i].hasEvent) {
                     m_dayCells[i].background.setFillColor(AppColors::CalendarEventBG);
                     m_dayCells[i].numberText.setFillColor(AppColors::CalendarEventDayText);
                     m_dayCells[i].numberText.setStyle(Text::Bold);
                 } else {
                    m_dayCells[i].background.setFillColor(AppColors::InputBoxBG);
                    m_dayCells[i].numberText.setFillColor(AppColors::CalendarDayText);
                    m_dayCells[i].numberText.setStyle(Text::Regular);
                }
                m_dayCells[i].numberText.setPosition(cellX + 6, cellY + 4);
                dayCounter++;
            } else {
                m_dayCells[i].background.setFillColor(AppColors::WidgetBackground);
                m_dayCells[i].numberText.setString("");
                m_dayCells[i].dayNumber = 0;
            }
        }
         m_eventDisplayText.setString(""); //When you select a date with an event, you might show the event details in m_eventDisplayText. After updating the calendar grid, it's useful to clear this text to avoid showing outdated or incorrect information.
    }

    int getDayFromPosition(const Vector2f& mousePos) const { //Tell us which day is clicked on the calendar grid
         float gridStartY = m_areaPosition.y + 95; //95 pixel offset due to headers above
         float dayCellWidth = m_areaSize.x / 7.0f;
         float gridEndY = m_eventDisplayText.getPosition().y - 15;
         float availableHeight = std::max(100.0f, gridEndY - gridStartY);
         float dayCellHeight = availableHeight / 6.0f;

         if (mousePos.x < m_areaPosition.x || mousePos.x > m_areaPosition.x + m_areaSize.x ||
             mousePos.y < gridStartY || mousePos.y > gridStartY + 6 * dayCellHeight) {
             return -1;
         }
         int col = static_cast<int>((mousePos.x - m_areaPosition.x) / dayCellWidth);
         int row = static_cast<int>((mousePos.y - gridStartY) / dayCellHeight);
         int index = row * 7 + col;
         if (index >= 0 && index < m_dayCells.size()) {
             return m_dayCells[index].dayNumber;
         }
        return -1;
    }

    Text m_monthYearText;
    unique_ptr<Button> m_prevMonthButton;
    unique_ptr<Button> m_nextMonthButton;
    vector<Text> m_dayHeaderTexts;
    vector<DayCell> m_dayCells;
    Text m_eventDisplayText;

    int m_currentYear;
    int m_currentMonth;
    map<tuple<int, int>, string> m_events;
};


// --- Stopwatch Widget ---
class StopwatchWidget : public Widget {
public:
    enum class State { Stopped, Running };
    StopwatchWidget() : m_state(State::Stopped), m_elapsedTime(Time::Zero) {}

     void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) override {
        Widget::setupUI(font, areaPosition, areaSize);
        m_timeDisplay.setFont(*m_font);
        m_timeDisplay.setCharacterSize(52);
        m_timeDisplay.setFillColor(AppColors::TextAccent1);
        updateDisplay();

        FloatRect textBounds = m_timeDisplay.getLocalBounds();
        m_timeDisplay.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top);
        m_timeDisplay.setPosition(areaPosition.x + areaSize.x / 2.0f, areaPosition.y + areaSize.y * 0.3f);

        float buttonY = areaPosition.y + areaSize.y * 0.7f;
        float buttonWidth = 120.0f;
        float buttonHeight = 45.0f;
        float buttonSpacing = 30.0f;
        float totalButtonAreaWidth = 3 * buttonWidth + 2 * buttonSpacing;
        float startX = areaPosition.x + (areaSize.x - totalButtonAreaWidth) / 2.0f;

        m_startButton = make_unique<Button>("Start", Vector2f(startX, buttonY), Vector2f(buttonWidth, buttonHeight), *m_font);
        m_stopButton = make_unique<Button>("Stop", Vector2f(startX + buttonWidth + buttonSpacing, buttonY), Vector2f(buttonWidth, buttonHeight), *m_font);
        m_resetButton = make_unique<Button>("Reset", Vector2f(startX + 2 * (buttonWidth + buttonSpacing), buttonY), Vector2f(buttonWidth, buttonHeight), *m_font);

        m_stopButton->setActive(false);
        m_resetButton->setActive(true);
    }

    void handleEvent(const Event& event, const RenderWindow& window) override {
         if (!m_isActive) return;
         m_startButton->handleEvent(event, window);
         m_stopButton->handleEvent(event, window);
         m_resetButton->handleEvent(event, window);

         if (m_startButton->isClicked()) { start(); }
         if (m_stopButton->isClicked()) { stop(); }
         if (m_resetButton->isClicked()) { reset(); }
    }

    void update(float dt) override {
        m_startButton->update();
        m_stopButton->update();
        m_resetButton->update();
        if (m_state == State::Running) {
            updateDisplay();
        }
    }

    void draw(RenderTarget& target) const override {
        target.draw(m_timeDisplay);
        m_startButton->draw(target);
        m_stopButton->draw(target);
        m_resetButton->draw(target);
    }

    void start() {
        if (m_state == State::Stopped) {
            m_state = State::Running;
            m_clock.restart();
            m_startButton->setActive(false);
            m_stopButton->setActive(true);
            m_resetButton->setActive(false);
        }
    }

    void stop() {
        if (m_state == State::Running) {
            m_state = State::Stopped;
            m_elapsedTime += m_clock.getElapsedTime();
            updateDisplay();
            m_startButton->setActive(true);
            m_stopButton->setActive(false);
            m_resetButton->setActive(true);
        }
    }

    void reset() {
        if(m_state == State::Stopped) {
            m_elapsedTime = Time::Zero;
            m_clock.restart();
            updateDisplay();
            m_startButton->setActive(true);
            m_stopButton->setActive(false);
            m_resetButton->setActive(true);
        }
    }

private:
    void updateDisplay() {
        Time timeToDisplay = m_elapsedTime;
        if (m_state == State::Running) {
            timeToDisplay += m_clock.getElapsedTime();
        }

        float totalSeconds = timeToDisplay.asSeconds();
        int hours = static_cast<int>(totalSeconds / 3600);
        int minutes = static_cast<int>(fmod(totalSeconds, 3600) / 60);
        int seconds = static_cast<int>(fmod(totalSeconds, 60));
        int milliseconds = static_cast<int>(fmod(timeToDisplay.asMicroseconds(), 1000000) / 1000);

        ostringstream oss;
        oss << setw(2) << setfill('0') << hours << ":"
             << setw(2) << setfill('0') << minutes << ":"
             << setw(2) << setfill('0') << seconds << "."
             << setw(3) << setfill('0') << milliseconds;

        m_timeDisplay.setString(oss.str());
        FloatRect textBounds = m_timeDisplay.getLocalBounds();
        m_timeDisplay.setOrigin(textBounds.left + textBounds.width / 2.0f, textBounds.top);
        m_timeDisplay.setPosition(m_areaPosition.x + m_areaSize.x / 2.0f, m_areaPosition.y + m_areaSize.y * 0.3f);
    }

    Text m_timeDisplay;
    unique_ptr<Button> m_startButton;
    unique_ptr<Button> m_stopButton;
    unique_ptr<Button> m_resetButton;

    State m_state;
    Clock m_clock;
    Time m_elapsedTime;
};

// --- Alarm Widget (with AM/PM) ---
class AlarmWidget : public Widget {
public:
    enum class State { Idle, SettingHour, SettingMinute, SettingAmPm, Set, Ringing, Snoozed };
    enum class AmPm { AM, PM };

    AlarmWidget() : m_state(State::Idle), m_alarmHour24(0), m_alarmMinute(0), m_alarmAmPm(AmPm::AM), m_snoozeMinutes(1)
    {
        if (!m_soundBuffer.loadFromFile("alarm.wav")) {
             cerr << "Warning: Could not load alarm sound 'alarm.wav'. Please ensure the file exists." << endl;
        } else {
            m_alarmSound.setBuffer(m_soundBuffer);
            m_alarmSound.setLoop(true);
        }
    }

     void setupUI(const Font& font, const Vector2f& areaPosition, const Vector2f& areaSize) override {
        Widget::setupUI(font, areaPosition, areaSize);
        m_statusText.setFont(*m_font);
        m_statusText.setCharacterSize(22);
        m_statusText.setFillColor(AppColors::TextDefault);
        m_statusText.setPosition(areaPosition.x + 20, areaPosition.y + 25);

        m_alarmTimeDisplay.setFont(*m_font);
        m_alarmTimeDisplay.setCharacterSize(40);
        m_alarmTimeDisplay.setFillColor(AppColors::TextAccent1);
        m_alarmTimeDisplay.setPosition(areaPosition.x + areaSize.x / 2.0f, areaPosition.y + 70);

         float inputY = areaPosition.y + 130;
         float boxWidth = 60;
         float boxHeight = 45;
         float spacing = 15;
         float totalInputWidth = boxWidth * 3 + spacing * 2;
         float inputStartX = areaPosition.x + (areaSize.x - totalInputWidth) / 2.0f;

         m_hourInputBox.setSize({boxWidth, boxHeight});
         m_hourInputBox.setPosition(inputStartX, inputY);
         m_hourInputBox.setFillColor(AppColors::InputBoxBG);
         m_hourInputBox.setOutlineColor(AppColors::InputBoxOutline);
         m_hourInputBox.setOutlineThickness(1);

         m_hourInputText.setFont(*m_font);
         m_hourInputText.setCharacterSize(26);
         m_hourInputText.setFillColor(AppColors::InputBoxText);
         m_hourInputText.setString("HH");
         centerTextInBox(m_hourInputText, m_hourInputBox);

         m_colon1Text.setFont(*m_font);
         m_colon1Text.setString(":");
         m_colon1Text.setCharacterSize(38);
         m_colon1Text.setFillColor(AppColors::TextDefault);
         m_colon1Text.setPosition(inputStartX + boxWidth + spacing/2.0f - m_colon1Text.getLocalBounds().width/2 , inputY + boxHeight/2 - m_colon1Text.getLocalBounds().height/1.5f );

         m_minuteInputBox.setSize({boxWidth, boxHeight});
         m_minuteInputBox.setPosition(inputStartX + boxWidth + spacing, inputY);
         m_minuteInputBox.setFillColor(AppColors::InputBoxBG);
         m_minuteInputBox.setOutlineColor(AppColors::InputBoxOutline);
         m_minuteInputBox.setOutlineThickness(1);

         m_minuteInputText.setFont(*m_font);
         m_minuteInputText.setCharacterSize(26);
         m_minuteInputText.setFillColor(AppColors::InputBoxText);
         m_minuteInputText.setString("MM");
         centerTextInBox(m_minuteInputText, m_minuteInputBox);

         m_amPmInputBox.setSize({boxWidth, boxHeight});
         m_amPmInputBox.setPosition(inputStartX + boxWidth*2 + spacing*2, inputY);
         m_amPmInputBox.setFillColor(AppColors::InputBoxBG);
         m_amPmInputBox.setOutlineColor(AppColors::InputBoxOutline);
         m_amPmInputBox.setOutlineThickness(1);

         m_amPmInputText.setFont(*m_font);
         m_amPmInputText.setCharacterSize(26);
         m_amPmInputText.setFillColor(AppColors::InputBoxText);
         m_amPmInputText.setString("AM");
         centerTextInBox(m_amPmInputText, m_amPmInputBox);

        float buttonY = areaPosition.y + areaSize.y - 90;
        float buttonWidth = 110.0f;
        float buttonHeight = 40.0f;
        float totalButtonAreaWidth = 3 * buttonWidth + 2 * spacing;
        float buttonStartX = areaPosition.x + (areaSize.x - totalButtonAreaWidth) / 2.0f;

        m_setButton = make_unique<Button>("Set Alarm", Vector2f(buttonStartX, buttonY), Vector2f(buttonWidth, buttonHeight), *m_font);
        m_cancelButton = make_unique<Button>("Cancel", Vector2f(buttonStartX + buttonWidth + spacing, buttonY), Vector2f(buttonWidth, buttonHeight), *m_font);
        m_snoozeButton = make_unique<Button>("Snooze", Vector2f(buttonStartX + 2*(buttonWidth + spacing), buttonY), Vector2f(buttonWidth, buttonHeight), *m_font);

        m_cancelButton->setActive(false);
        m_snoozeButton->setActive(false);
        updateStatusDisplay();
    }

    void activate() override {
        Widget::activate();
        cancel();
    }

    void deactivate() override {
        Widget::deactivate();
        if (m_soundBuffer.getSampleCount() > 0 && m_alarmSound.getStatus() == Sound::Playing) {
            m_alarmSound.stop();
         }
        m_hourInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_minuteInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_amPmInputBox.setOutlineColor(AppColors::InputBoxOutline);
    }

    bool isAcceptingTextInput() const override {
        return m_state == State::SettingHour || m_state == State::SettingMinute;
    }

    void handleEvent(const Event& event, const RenderWindow& window) override {
        if (!m_isActive) return;

        m_setButton->handleEvent(event, window);
        m_cancelButton->handleEvent(event, window);
        m_snoozeButton->handleEvent(event, window);

        switch(m_state) {
            case State::Ringing:
                if (m_snoozeButton->isClicked()) { snooze(); return; }
                if (m_cancelButton->isClicked()) { cancel(); return; }
                if (event.type == Event::KeyPressed ||
                   (event.type == Event::MouseButtonPressed &&
                    !m_snoozeButton->getGlobalBounds().contains(window.mapPixelToCoords(Mouse::getPosition(window))) &&
                    !m_cancelButton->getGlobalBounds().contains(window.mapPixelToCoords(Mouse::getPosition(window))) ))
                {
                     cancel(); return;
                }
                break;

            case State::Set:
            case State::Snoozed:
                if (m_cancelButton->isClicked()) { cancel(); return; }
                if (m_setButton->isClicked()) { startSetting(); return; }
                break;

            case State::Idle:
                 if (m_setButton->isClicked()) { startSetting(); return; }
                 handleInputBoxClicks(event, window);
                 break;

            case State::SettingHour:
            case State::SettingMinute:
                if (event.type == Event::TextEntered) { handleNumericInput(event.text.unicode); return; }
                if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) { cancelInput(); return; }
                if (m_cancelButton->isClicked()) { cancelInput(); return; }
                handleInputBoxClicks(event, window);
                if (m_setButton->isClicked()) { finalizeSettingFromNumeric(); return; }
                break;

            case State::SettingAmPm:
                 if (event.type == Event::KeyPressed) {
                     if (event.key.code == Keyboard::A) { m_alarmAmPm = AmPm::AM; m_amPmInputText.setString("AM"); centerTextInBox(m_amPmInputText, m_amPmInputBox); }
                     else if (event.key.code == Keyboard::P) { m_alarmAmPm = AmPm::PM; m_amPmInputText.setString("PM"); centerTextInBox(m_amPmInputText, m_amPmInputBox); }
                     else if (event.key.code == Keyboard::Enter || event.key.code == Keyboard::Return) { setAlarm(); return; }
                     else if (event.key.code == Keyboard::Escape) { cancelInput(); return; }
                 }
                 if (event.type == Event::MouseButtonPressed) {
                     Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
                     if (m_amPmInputBox.getGlobalBounds().contains(mousePos)) {
                         m_alarmAmPm = (m_alarmAmPm == AmPm::AM) ? AmPm::PM : AmPm::AM;
                         m_amPmInputText.setString((m_alarmAmPm == AmPm::AM) ? "AM" : "PM");
                         centerTextInBox(m_amPmInputText, m_amPmInputBox);
                     } else if (m_setButton->getGlobalBounds().contains(mousePos)) {
                         setAlarm(); return;
                     } else {
                         handleInputBoxClicks(event, window);
                     }
                 }
                 if (m_cancelButton->isClicked()) { cancelInput(); return; }
                 if (m_setButton->isClicked()) { setAlarm(); return; }
                 break;
        }
    }

    void update(float dt) override {
        m_setButton->update();
        m_cancelButton->update();
        m_snoozeButton->update();

        if (m_state == State::Set || m_state == State::Snoozed) {
            auto now = chrono::system_clock::now();
            auto now_c = chrono::system_clock::to_time_t(now);
            tm now_tm = *localtime(&now_c);

            if (now_tm.tm_hour == m_alarmHour24 && now_tm.tm_min == m_alarmMinute) {
                if (m_state != State::Ringing) {
                    bool alreadyTriggered = false;
                    if (m_lastTriggerTime.time_since_epoch().count() > 0) {
                        auto lastTrigger_c = chrono::system_clock::to_time_t(m_lastTriggerTime);
                        tm lastTrigger_tm = *localtime(&lastTrigger_c);
                        if(lastTrigger_tm.tm_year == now_tm.tm_year &&
                           lastTrigger_tm.tm_yday == now_tm.tm_yday &&
                           lastTrigger_tm.tm_hour == now_tm.tm_hour &&
                           lastTrigger_tm.tm_min == now_tm.tm_min) {
                            alreadyTriggered = true;
                        }
                    }
                    if (!alreadyTriggered) {
                        m_state = State::Ringing;
                        m_lastTriggerTime = now;
                        updateStatusDisplay();
                        if (m_soundBuffer.getSampleCount() > 0 && m_alarmSound.getStatus() != Sound::Playing) {
                            m_alarmSound.play();
                         }
                        m_ringVisualClock.restart();
                        m_snoozeButton->setActive(true);
                        m_setButton->setActive(false);
                        m_cancelButton->setActive(true);
                    }
                }
            }
        }

        if (m_state == State::Ringing) {
            if (m_ringVisualClock.getElapsedTime().asSeconds() > 0.5f) {
                m_alarmTimeDisplay.setFillColor(m_alarmTimeDisplay.getFillColor() == AppColors::AlarmRingingFlash1 ? AppColors::AlarmRingingFlash2 : AppColors::AlarmRingingFlash1);
                m_ringVisualClock.restart();
            }
        } else {
             if (m_state != State::SettingHour && m_state != State::SettingMinute && m_state != State::SettingAmPm) {
                m_alarmTimeDisplay.setFillColor(AppColors::TextAccent1);
             }
        }

         static Clock cursorClock;
         if (m_state == State::SettingHour || m_state == State::SettingMinute) {
            if (cursorClock.getElapsedTime().asSeconds() > 0.5f) {
                  Text* targetText = (m_state == State::SettingHour) ? &m_hourInputText : &m_minuteInputText;
                  string currentDisplay = targetText->getString();
                  if (!currentDisplay.empty() && currentDisplay.length() > m_currentInput.length() && currentDisplay.back() == '_') {
                     targetText->setString(m_currentInput);
                  } else {
                     targetText->setString(m_currentInput + "_");
                  }
                  centerTextInBox(*targetText, (m_state == State::SettingHour) ? m_hourInputBox : m_minuteInputBox);
                  cursorClock.restart();
            }
         } else {
              if (m_hourInputText.getString().find('_') != string::npos) {
                  m_hourInputText.setString( (m_tempHour12 == 0 && (m_state == State::Idle || m_state == State::SettingHour)) ? "HH" : to_string(m_tempHour12));
                  centerTextInBox(m_hourInputText, m_hourInputBox);
              }
              if (m_minuteInputText.getString().find('_') != string::npos) {
                   ostringstream minStream;
                   minStream << setw(2) << setfill('0') << m_alarmMinute;
                   m_minuteInputText.setString( (m_tempHour12 == 0 && m_alarmMinute == 0 && (m_state == State::Idle || m_state == State::SettingMinute)) ? "MM" : minStream.str());
                   centerTextInBox(m_minuteInputText, m_minuteInputBox);
              }
        }
    }

    void draw(RenderTarget& target) const override {
        target.draw(m_statusText);
        target.draw(m_alarmTimeDisplay);
        target.draw(m_hourInputBox);
        target.draw(m_minuteInputBox);
        target.draw(m_amPmInputBox);
        target.draw(m_colon1Text);
        target.draw(m_hourInputText);
        target.draw(m_minuteInputText);
        target.draw(m_amPmInputText);
        m_setButton->draw(target);
        m_cancelButton->draw(target);
        m_snoozeButton->draw(target);
    }

private:
    void centerTextInBox(Text& text, const RectangleShape& box) {
         FloatRect textBounds = text.getLocalBounds();
         text.setOrigin(std::floor(textBounds.left + textBounds.width / 2.0f), std::floor(textBounds.top + textBounds.height / 2.0f));
         text.setPosition(std::floor(box.getPosition().x + box.getSize().x / 2.0f), std::floor(box.getPosition().y + box.getSize().y / 2.0f));
    }

    void handleInputBoxClicks(const Event& event, const RenderWindow& window) {
         if (event.type == Event::MouseButtonPressed) {
              Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window));
              if (m_hourInputBox.getGlobalBounds().contains(mousePos)) { startSetting(State::SettingHour); }
              else if (m_minuteInputBox.getGlobalBounds().contains(mousePos)) { startSetting(State::SettingMinute); }
              else if (m_amPmInputBox.getGlobalBounds().contains(mousePos)) { startSetting(State::SettingAmPm); }
         }
    }

     void finalizeSettingFromNumeric() {
         bool canSet = false;
         if (m_state == State::SettingHour) {
             if (!m_currentInput.empty()) {
                 try {
                     int hour12 = stoi(m_currentInput);
                     if (hour12 >= 1 && hour12 <= 12) {
                        m_tempHour12 = hour12;
                        canSet = true;
                        startSetting(State::SettingMinute); return;
                     }
                 } catch (...) { /* stoi failed */ }
             }
         } else if (m_state == State::SettingMinute) {
             if (!m_currentInput.empty() && m_tempHour12 != 0) {
                 try {
                     int minute = stoi(m_currentInput);
                     if (minute >= 0 && minute <= 59) {
                        m_alarmMinute = minute;
                        canSet = true;
                        startSetting(State::SettingAmPm); return;
                     }
                 } catch (...) { /* stoi failed */ }
             }
         }
         if (canSet) {
             setAlarm();
         } else {
             cancelInput();
         }
     }

    void handleNumericInput(Uint32 unicode) {
         if (unicode >= '0' && unicode <= '9') {
             int maxLen = 2;
            if (m_currentInput.length() < maxLen) {
                 m_currentInput += static_cast<char>(unicode);
                 if (m_state == State::SettingHour) {
                    m_hourInputText.setString(m_currentInput + "_");
                    centerTextInBox(m_hourInputText, m_hourInputBox);
                    if (m_currentInput.length() >= maxLen) { advanceFromHourInput(); }
                 } else {
                    m_minuteInputText.setString(m_currentInput + "_");
                    centerTextInBox(m_minuteInputText, m_minuteInputBox);
                    if (m_currentInput.length() >= maxLen) { advanceFromMinuteInput(); }
                 }
             }
         } else if (unicode == '\b') {
              if (!m_currentInput.empty()) {
                 m_currentInput.pop_back();
                 if (m_state == State::SettingHour) { m_hourInputText.setString(m_currentInput + "_"); centerTextInBox(m_hourInputText, m_hourInputBox); }
                 else { m_minuteInputText.setString(m_currentInput + "_"); centerTextInBox(m_minuteInputText, m_minuteInputBox); }
              }
        } else if (unicode == '\r' || unicode == '\n') {
              if (m_state == State::SettingHour) { advanceFromHourInput(true); }
              else if (m_state == State::SettingMinute) { advanceFromMinuteInput(true); }
         }
    }

    void advanceFromHourInput(bool forceAdvance = false) {
        if (!m_currentInput.empty()) {
            try {
                int hour12 = stoi(m_currentInput);
                if (hour12 >= 1 && hour12 <= 12) {
                    m_tempHour12 = hour12;
                    m_hourInputText.setString(to_string(m_tempHour12));
                    centerTextInBox(m_hourInputText, m_hourInputBox);
                    startSetting(State::SettingMinute);
                    return;
                }
            } catch(...) { /* Conversion failed */ }
        }
        if (forceAdvance || m_currentInput.length() >= 2) {
            m_currentInput = "";
            m_hourInputText.setString("HH_");
            centerTextInBox(m_hourInputText, m_hourInputBox);
            m_tempHour12 = 0;
        }
    }

    void advanceFromMinuteInput(bool forceAdvance = false) {
         if (!m_currentInput.empty()) {
             try {
                 int minute = stoi(m_currentInput);
                 if (minute >= 0 && minute <= 59) {
                     m_alarmMinute = minute;
                     ostringstream minStream;
                     minStream << setw(2) << setfill('0') << m_alarmMinute;
                     m_minuteInputText.setString(minStream.str());
                     centerTextInBox(m_minuteInputText, m_minuteInputBox);
                     startSetting(State::SettingAmPm);
                     return;
                 }
             } catch(...) { /* Conversion failed */ }
         }
          if (forceAdvance || m_currentInput.length() >= 2) {
             m_currentInput = "";
             m_minuteInputText.setString("MM_");
             centerTextInBox(m_minuteInputText, m_minuteInputBox);
             m_alarmMinute = 0;
          }
    }

    void startSetting(State targetState = State::SettingHour) {
        bool isStartingFresh = (m_state == State::Idle || m_state == State::Set || m_state == State::Snoozed);

        if (isStartingFresh && targetState == State::SettingHour) {
            m_tempHour12 = 0;
            m_alarmMinute = 0;
            m_alarmAmPm = AmPm::AM;
        }

        if ((targetState == State::SettingMinute || targetState == State::SettingAmPm) && m_tempHour12 == 0 && !isStartingFresh) {
             targetState = State::SettingHour;
        }

        m_state = targetState;
        m_currentInput = "";

        m_hourInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_minuteInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_amPmInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_hourInputText.setFillColor(AppColors::InputBoxText);
        m_minuteInputText.setFillColor(AppColors::InputBoxText);
        m_amPmInputText.setFillColor(AppColors::InputBoxText);

        m_hourInputText.setString(m_tempHour12 == 0 ? "HH" : to_string(m_tempHour12));
        ostringstream minStream;
        minStream << setw(2) << setfill('0') << m_alarmMinute;
        m_minuteInputText.setString((m_tempHour12 == 0 && m_alarmMinute == 0 && targetState != State::SettingAmPm) ? "MM" : minStream.str());
        m_amPmInputText.setString(m_alarmAmPm == AmPm::AM ? "AM" : "PM");

        centerTextInBox(m_hourInputText, m_hourInputBox);
        centerTextInBox(m_minuteInputText, m_minuteInputBox);
        centerTextInBox(m_amPmInputText, m_amPmInputBox);

        switch(targetState) {
            case State::SettingHour:
                m_hourInputBox.setOutlineColor(AppColors::TextAccent1);
                m_hourInputText.setString("_");
                centerTextInBox(m_hourInputText, m_hourInputBox);
                if (isStartingFresh) {
                    m_minuteInputText.setString("MM"); centerTextInBox(m_minuteInputText, m_minuteInputBox);
                    m_amPmInputText.setString("AM"); centerTextInBox(m_amPmInputText, m_amPmInputBox);
                }
                break;
            case State::SettingMinute:
                m_minuteInputBox.setOutlineColor(AppColors::TextAccent1);
                m_minuteInputText.setString("_");
                centerTextInBox(m_minuteInputText, m_minuteInputBox);
                break;
            case State::SettingAmPm:
                m_amPmInputBox.setOutlineColor(AppColors::TextAccent1);
                break;
            default: break;
        }
        m_cancelButton->setActive(true);
        m_setButton->setActive(true);
        m_snoozeButton->setActive(false);
        updateStatusDisplay();
    }

    void updateStatusDisplay() {
        ostringstream status;
        string alarmTimeStr = "--:-- --";

        if (m_state == State::Set || m_state == State::Snoozed || m_state == State::Ringing) {
             alarmTimeStr = formatTime12Hour(m_alarmHour24, m_alarmMinute);
        } else if (m_state == State::SettingAmPm || m_state == State::SettingMinute) {
            if (m_tempHour12 != 0) {
                 ostringstream tempTime;
                 tempTime << setw(2) << setfill('0') << m_tempHour12 << ":"
                          << setw(2) << setfill('0') << m_alarmMinute
                          << (m_alarmAmPm == AmPm::AM ? " AM" : " PM");
                 alarmTimeStr = tempTime.str();
            }
        }

        switch (m_state) {
            case State::Idle:
                status << "Alarm: Idle. Click 'Set Alarm' or input boxes.";
                m_hourInputText.setString("HH");
                m_minuteInputText.setString("MM");
                m_amPmInputText.setString("AM");
                alarmTimeStr = "--:-- --";
                break;
            case State::SettingHour:
                status << "Enter Hour (1-12), then Enter or click MM.";
                alarmTimeStr = (m_currentInput.empty() ? "__" : m_currentInput) + ":-- --";
                break;
            case State::SettingMinute:
                 status << "Enter Minute (0-59), then Enter or click AM/PM.";
                 alarmTimeStr = (m_tempHour12 == 0 ? "--" : to_string(m_tempHour12)) + ":" + (m_currentInput.empty() ? "__" : m_currentInput) + " --";
                 break;
            case State::SettingAmPm:
                 status << "Select AM/PM (A/P keys or click box), then Enter or Set.";
                 break;
            case State::Set:
                 status << "Alarm Set For:";
                 break;
            case State::Ringing:
                 status << "ALARM RINGING! Stop or Snooze.";
                 break;
            case State::Snoozed:
                 status << "Alarm Snoozed Until:";
                 break;
        }
        m_statusText.setString(status.str());
        m_alarmTimeDisplay.setString(alarmTimeStr);
        FloatRect bounds = m_alarmTimeDisplay.getLocalBounds();
        m_alarmTimeDisplay.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top);
        m_alarmTimeDisplay.setPosition(m_areaPosition.x + m_areaSize.x / 2.0f, m_areaPosition.y + 70);

        if (m_state != State::SettingHour && m_state != State::SettingMinute && m_state != State::SettingAmPm) {
             m_hourInputBox.setOutlineColor(AppColors::InputBoxOutline);
             m_minuteInputBox.setOutlineColor(AppColors::InputBoxOutline);
             m_amPmInputBox.setOutlineColor(AppColors::InputBoxOutline);
        }
        if (m_state == State::Set || m_state == State::Snoozed || m_state == State::Ringing || m_state == State::Idle) {
            int dispH = m_alarmHour24 % 12; if (dispH == 0 && m_alarmHour24 !=0) dispH = 12; else if (m_alarmHour24 == 0 && m_state == State::Idle) dispH = 0;
            m_hourInputText.setString( (m_state == State::Idle && m_alarmHour24 == 0) ? "HH" : to_string(dispH));
            ostringstream minStr; minStr << setw(2) << setfill('0') << m_alarmMinute;
            m_minuteInputText.setString( (m_state == State::Idle && m_alarmHour24 == 0 && m_alarmMinute == 0) ? "MM" : minStr.str());
            m_amPmInputText.setString( (m_alarmHour24 < 12 || m_alarmHour24 == 24) ? "AM" : "PM");
            if (m_state == State::Idle && m_alarmHour24 == 0 && m_alarmMinute == 0) m_amPmInputText.setString("AM");

            centerTextInBox(m_hourInputText, m_hourInputBox);
            centerTextInBox(m_minuteInputText, m_minuteInputBox);
            centerTextInBox(m_amPmInputText, m_amPmInputBox);
        }
    }

    void setAlarm() {
         if (m_tempHour12 == 0) {
            cerr << "Error: Cannot set alarm without valid hour." << endl;
            startSetting(State::SettingHour);
            return;
         }
         if (m_alarmMinute < 0 || m_alarmMinute > 59) {
             cerr << "Error: Cannot set alarm with invalid minute." << endl;
             startSetting(State::SettingMinute);
             return;
         }

         if (m_alarmAmPm == AmPm::AM) {
             m_alarmHour24 = (m_tempHour12 == 12) ? 0 : m_tempHour12;
         } else {
             m_alarmHour24 = (m_tempHour12 == 12) ? 12 : m_tempHour12 + 12;
         }

         m_state = State::Set;
         m_lastTriggerTime = {};
         m_currentInput = "";
         m_setButton->setActive(true);
         m_cancelButton->setActive(true);
         m_snoozeButton->setActive(false);
         updateStatusDisplay();
    }

    void cancelInput() {
        // State prevStateBeforeInput = State::Idle; // Not used currently
        if (m_alarmHour24 != 0 || m_alarmMinute != 0) {
             if (m_soundBuffer.getSampleCount() > 0 && m_alarmSound.getStatus() == Sound::Playing) {
                m_alarmSound.stop();
             }
             m_state = State::Set;
        } else {
            m_state = State::Idle;
        }

        m_currentInput = "";
        m_setButton->setActive(true);
        m_cancelButton->setActive(m_state != State::Idle);
        m_snoozeButton->setActive(m_state == State::Ringing);
        updateStatusDisplay();
        m_alarmTimeDisplay.setFillColor(AppColors::TextAccent1);
        m_hourInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_minuteInputBox.setOutlineColor(AppColors::InputBoxOutline);
        m_amPmInputBox.setOutlineColor(AppColors::InputBoxOutline);
    }

     void cancel() {
        m_state = State::Idle;
        if (m_soundBuffer.getSampleCount() > 0 && m_alarmSound.getStatus() == Sound::Playing) {
            m_alarmSound.stop();
        }
        m_alarmHour24 = 0;
        m_alarmMinute = 0;
        m_tempHour12 = 0;
        m_alarmAmPm = AmPm::AM;
        m_currentInput = "";
        m_lastTriggerTime = {};

        m_setButton->setActive(true);
        m_cancelButton->setActive(false);
        m_snoozeButton->setActive(false);
        updateStatusDisplay();
        m_alarmTimeDisplay.setFillColor(AppColors::TextAccent1);
    }

     void snooze() {
         if (m_state == State::Ringing) {
             if (m_soundBuffer.getSampleCount() > 0 && m_alarmSound.getStatus() == Sound::Playing) {
                 m_alarmSound.stop();
             }
             auto now = chrono::system_clock::now();
             auto snoozeUntil = now + chrono::minutes(m_snoozeMinutes);
             auto snoozeUntil_c = chrono::system_clock::to_time_t(snoozeUntil);
             tm snooze_tm = *localtime(&snoozeUntil_c);

             m_alarmHour24 = snooze_tm.tm_hour;
             m_alarmMinute = snooze_tm.tm_min;
             m_tempHour12 = m_alarmHour24 % 12;
             if (m_tempHour12 == 0) m_tempHour12 = 12;
             m_alarmAmPm = (m_alarmHour24 < 12 || m_alarmHour24 == 24) ? AmPm::AM : AmPm::PM;

             m_state = State::Snoozed;
             m_lastTriggerTime = {};
             m_snoozeButton->setActive(false);
             m_cancelButton->setActive(true);
             m_setButton->setActive(true);
             updateStatusDisplay();
             m_alarmTimeDisplay.setFillColor(AppColors::TextAccent1);
        }
    }

    State m_state;
    int m_alarmHour24;
    int m_alarmMinute;
    AmPm m_alarmAmPm;
    int m_tempHour12;

    Text m_statusText;
    Text m_alarmTimeDisplay;
    RectangleShape m_hourInputBox, m_minuteInputBox, m_amPmInputBox;
    Text m_colon1Text;
    Text m_hourInputText, m_minuteInputText, m_amPmInputText;
    string m_currentInput;

    unique_ptr<Button> m_setButton, m_cancelButton, m_snoozeButton;
    SoundBuffer m_soundBuffer;
    Sound m_alarmSound;
    Clock m_ringVisualClock;
    int m_snoozeMinutes;
    chrono::system_clock::time_point m_lastTriggerTime;
};

// --- Main Application Class ---
class App {
public:
    App() : m_window(VideoMode(static_cast<unsigned int>(WINDOW_WIDTH), static_cast<unsigned int>(WINDOW_HEIGHT)), "Productivity Dashboard++ V7"),
            m_activeWidgetIndex(-1),
            m_noteManager("notes_v2.txt"),
            m_notesAccessGranted(false),      // Initialize password state
            m_attemptingNotesAccess(false)  // Initialize password state
    {
        m_window.setFramerateLimit(60);
        if (!globalFont.loadFromFile("arial.ttf")) {
            cerr << "FATAL ERROR: Could not load font 'arial.ttf'! Make sure it's in the same directory." << endl;
            throw runtime_error("Failed to load font.");
        }
        // Load background texture
        if (!m_backgroundTexture.loadFromFile("background.png")) {
            cerr << "Warning: Could not load 'background.png'. Ensure it's in the same directory as the executable." << endl;
            // Proceed without background image
        } else {
            m_backgroundSprite.setTexture(m_backgroundTexture);
            // Optional: Scale sprite to fit window, maintaining aspect ratio or stretching
            // This example stretches the image to fill the window:
            m_backgroundSprite.setScale(
                WINDOW_WIDTH / m_backgroundSprite.getLocalBounds().width,
                WINDOW_HEIGHT / m_backgroundSprite.getLocalBounds().height
            );
        }
        setupUI();
    }

    void run() {
        Clock deltaClock;
        while (m_window.isOpen()) {
            Time dt = deltaClock.restart();
            handleEvents();
            update(dt.asSeconds());
            render();
        }
    }

private:
    void setupUI() {
        float navHeight = 50.0f;
        float statusHeight = 35.0f;
        float widgetHeight = WINDOW_HEIGHT - navHeight - statusHeight;

        m_navArea.setSize({WINDOW_WIDTH, navHeight});
        m_navArea.setFillColor(AppColors::NavBackground);
        m_navArea.setPosition(0, 0);

        m_widgetArea.setSize({WINDOW_WIDTH, widgetHeight});
        m_widgetArea.setFillColor(AppColors::WidgetBackground);
        m_widgetArea.setPosition(0, navHeight);

        m_statusArea.setSize({WINDOW_WIDTH, statusHeight});
        m_statusArea.setFillColor(AppColors::StatusBackground);
        m_statusArea.setPosition(0, navHeight + widgetHeight);

        m_navItemsData = {"1: Clock", "2: Weather", "3: Notes", "4: Calendar", "5: Alarm", "6: Stopwatch"};
        m_navItems.resize(m_navItemsData.size());
        float itemWidth = WINDOW_WIDTH / static_cast<float>(m_navItemsData.size());
        for (size_t i = 0; i < m_navItemsData.size(); ++i) {
            m_navItems[i].background.setSize({itemWidth, navHeight});
            m_navItems[i].background.setPosition(itemWidth * i, 0);
            m_navItems[i].background.setFillColor(Color::Transparent);
            m_navItems[i].background.setOutlineColor(AppColors::BackgroundDark);
            m_navItems[i].background.setOutlineThickness(1);

            m_navItems[i].label.setFont(globalFont);
            m_navItems[i].label.setString(m_navItemsData[i]);
            m_navItems[i].label.setCharacterSize(18);
            m_navItems[i].label.setFillColor(AppColors::NavInactiveText);

            FloatRect textBounds = m_navItems[i].label.getLocalBounds();
            m_navItems[i].label.setOrigin(std::floor(textBounds.left + textBounds.width / 2.0f), std::floor(textBounds.top + textBounds.height / 2.0f));
            m_navItems[i].label.setPosition(std::floor(itemWidth * i + itemWidth / 2.0f), std::floor(navHeight / 2.0f));
        }

         m_statusLabel.setFont(globalFont);
         m_statusLabel.setCharacterSize(16);
         m_statusLabel.setFillColor(AppColors::TextPrompt);
         m_statusLabel.setPosition(15, navHeight + widgetHeight + 8);
         m_statusLabel.setString("Welcome! Select a widget using keys 1-6.");

        // Password UI elements
        m_passwordPromptText.setFont(globalFont);
        m_passwordPromptText.setCharacterSize(24);
        m_passwordPromptText.setFillColor(AppColors::TextHeader);
        m_passwordPromptText.setString("Enter Password for Notes:");

        m_passwordInputDisplay.setFont(globalFont);
        m_passwordInputDisplay.setCharacterSize(24);
        m_passwordInputDisplay.setFillColor(AppColors::TextDefault);
        m_passwordInputDisplay.setString(""); // Initially empty

        m_passwordMessageText.setFont(globalFont);
        m_passwordMessageText.setCharacterSize(18);
        m_passwordMessageText.setFillColor(AppColors::TextWarning); // For "Incorrect Password"
        m_passwordMessageText.setString(""); // Initially empty


        m_mainWidgets.push_back(make_unique<ClockWidget>());
        m_mainWidgets.push_back(make_unique<WeatherWidget>());
        m_mainWidgets.push_back(make_unique<NotesWidget>(m_noteManager));
        m_mainWidgets.push_back(make_unique<CalendarWidget>());
        m_mainWidgets.push_back(make_unique<AlarmWidget>());
        m_mainWidgets.push_back(make_unique<StopwatchWidget>());

        Vector2f widgetAreaPos = m_widgetArea.getPosition();
        Vector2f widgetAreaSize = m_widgetArea.getSize();
        for (auto& widget : m_mainWidgets) {
            widget->setupUI(globalFont, widgetAreaPos, widgetAreaSize);
        }
        activateWidget(0); // Activate Clock widget by default
    }

    void handleEvents() {
        Event event;
        while (m_window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                m_window.close();
            }

            // Handle password input if attempting to access notes
            if (m_attemptingNotesAccess && m_activeWidgetIndex == 2) { // Notes widget is index 2
                if (event.type == Event::TextEntered) {
                    if (event.text.unicode == '\b') { // Backspace
                        if (!m_actualPasswordInput.empty()) {
                            m_actualPasswordInput.pop_back();
                        }
                    } else if (event.text.unicode == '\r' || event.text.unicode == '\n') { // Enter
                        if (m_actualPasswordInput == NOTES_PASSWORD) {
                            m_notesAccessGranted = true;
                            m_attemptingNotesAccess = false;
                            m_mainWidgets[m_activeWidgetIndex]->activate(); // Fully activate Notes
                            m_statusLabel.setString("Notes: Click heading to select/edit. Scroll wheel. Add/Edit/Delete notes.");
                            m_passwordMessageText.setString("");
                        } else {
                            m_passwordMessageText.setString("Incorrect Password. Try again.");
                            m_actualPasswordInput.clear();
                        }
                    } else if (event.text.unicode >= 32 && event.text.unicode < 127) { // Printable chars
                        if (m_actualPasswordInput.length() < 50) { // Limit password length
                            m_actualPasswordInput += static_cast<char>(event.text.unicode);
                        }
                    }
                    string asterisks(m_actualPasswordInput.length(), '*');
                    m_passwordInputDisplay.setString(asterisks);
                } else if (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape) {
                    m_attemptingNotesAccess = false;
                    m_actualPasswordInput.clear();
                    m_passwordInputDisplay.setString("");
                    m_passwordMessageText.setString("");
                    activateWidget(0); // Go to Clock widget or previously active
                }
                // Event consumed by password input logic, so return.
                // This prevents other handlers (like widget-specific or global key presses) from firing.
                continue;
            }


            // Global key presses for widget navigation (if not in text input mode of a widget)
            bool widgetNeedsInput = false;
            if (m_activeWidgetIndex >= 0 && m_activeWidgetIndex < m_mainWidgets.size()) {
                 if (m_mainWidgets[m_activeWidgetIndex]->isActive()) { // Check if widget is active
                    widgetNeedsInput = m_mainWidgets[m_activeWidgetIndex]->isAcceptingTextInput();
                 }
            }
             bool isNotesConfirmDelete = false;
             if (m_activeWidgetIndex == 2 && m_notesAccessGranted) { // Notes widget is index 2
                NotesWidget* notesWidget = static_cast<NotesWidget*>(m_mainWidgets[m_activeWidgetIndex].get());
                if (notesWidget->m_inputState == NotesWidget::InputState::ConfirmDelete) {
                    isNotesConfirmDelete = true;
                }
            }


            if (!widgetNeedsInput && !isNotesConfirmDelete && event.type == Event::KeyPressed) {
                int targetWidget = -1;
                if (event.key.code >= Keyboard::Num1 && event.key.code <= Keyboard::Num6) {
                    targetWidget = event.key.code - Keyboard::Num1;
                 } else if (event.key.code >= Keyboard::Numpad1 && event.key.code <= Keyboard::Numpad6) {
                    targetWidget = event.key.code - Keyboard::Numpad1;
                }

                if (targetWidget != -1 && targetWidget < m_mainWidgets.size()) {
                    activateWidget(targetWidget);
                    continue;
                } else if (event.key.code == Keyboard::Escape) {
                     m_window.close();
                     continue;
                }
            }

            // Pass event to the active widget if it's not password entry mode for notes
            if (m_activeWidgetIndex >= 0 && m_activeWidgetIndex < m_mainWidgets.size()) {
                // Only pass to NotesWidget if access is granted
                if (m_activeWidgetIndex == 2 && !m_notesAccessGranted) {
                    // Do nothing, password prompt is handling events
                } else {
                   if (m_mainWidgets[m_activeWidgetIndex]->isActive()) {
                       m_mainWidgets[m_activeWidgetIndex]->handleEvent(event, m_window);
                   }
                }
            }
        }
    }

    void update(float dt) {
        if (m_attemptingNotesAccess && m_activeWidgetIndex == 2) {
            // Update password prompt UI elements (e.g., centering text)
            Vector2f widgetAreaPos = m_widgetArea.getPosition();
            Vector2f widgetAreaSize = m_widgetArea.getSize();
            float currentY = widgetAreaPos.y + widgetAreaSize.y / 2.0f - 60; // Start Y pos for prompt

            m_passwordPromptText.setPosition(
                widgetAreaPos.x + (widgetAreaSize.x - m_passwordPromptText.getLocalBounds().width) / 2.0f,
                currentY
            );
            currentY += m_passwordPromptText.getLocalBounds().height + 20;

            m_passwordInputDisplay.setPosition(
                widgetAreaPos.x + (widgetAreaSize.x - m_passwordInputDisplay.getLocalBounds().width) / 2.0f,
                currentY
            );
            currentY += m_passwordInputDisplay.getLocalBounds().height + 15;

            m_passwordMessageText.setPosition(
                widgetAreaPos.x + (widgetAreaSize.x - m_passwordMessageText.getLocalBounds().width) / 2.0f,
                currentY
            );
        } else if (m_activeWidgetIndex >= 0 && m_activeWidgetIndex < m_mainWidgets.size()) {
            if (m_mainWidgets[m_activeWidgetIndex]->isActive()) {
                 m_mainWidgets[m_activeWidgetIndex]->update(dt);
            }
        }
    }

    void render() {
        m_window.clear(AppColors::BackgroundDark); // Clear with a fallback color

        // Draw background image if texture is loaded
        if (m_backgroundTexture.getSize().x > 0) { // Check if texture was loaded
            m_window.draw(m_backgroundSprite);
        }

        m_window.draw(m_navArea);
        m_window.draw(m_widgetArea); // Draw widget area background (can be semi-transparent)
        m_window.draw(m_statusArea);

        for (const auto& item : m_navItems) {
            m_window.draw(item.background);
            m_window.draw(item.label);
        }
        m_window.draw(m_statusLabel);

        if (m_attemptingNotesAccess && m_activeWidgetIndex == 2) {
            m_window.draw(m_passwordPromptText);
            m_window.draw(m_passwordInputDisplay);
            m_window.draw(m_passwordMessageText);
        } else if (m_activeWidgetIndex >= 0 && m_activeWidgetIndex < m_mainWidgets.size()) {
            if (m_activeWidgetIndex == 2 && !m_notesAccessGranted) {
                // This case should ideally be covered by m_attemptingNotesAccess
                // If somehow reached, draw password prompt again or a locked message
                 m_window.draw(m_passwordPromptText); // Fallback
                 m_window.draw(m_passwordInputDisplay);
                 m_window.draw(m_passwordMessageText);
            } else {
                if(m_mainWidgets[m_activeWidgetIndex]->isActive()){
                    m_mainWidgets[m_activeWidgetIndex]->draw(m_window);
                }
            }
        }
        m_window.display();
    }

    void activateWidget(int index) {
        if (index < 0 || index >= m_mainWidgets.size()) { // Invalid index
            return;
        }
        
        // If navigating away from Notes, lock it
        if (m_activeWidgetIndex == 2 && index != 2) {
            m_notesAccessGranted = false;
            m_attemptingNotesAccess = false; // Reset attempt state
             if(m_mainWidgets[2]->isActive()) m_mainWidgets[2]->deactivate(); // Deactivate notes widget explicitly
        }


        // Deactivate previously active widget (if any and valid)
        if (m_activeWidgetIndex >= 0 && m_activeWidgetIndex < m_mainWidgets.size() && m_activeWidgetIndex != index) {
            if(m_mainWidgets[m_activeWidgetIndex]->isActive()) {
                m_mainWidgets[m_activeWidgetIndex]->deactivate();
            }
        }
        
        int oldActiveWidgetIndex = m_activeWidgetIndex;
        m_activeWidgetIndex = index;

        if (index == 2) { // Notes widget (index 2)
            if (!m_notesAccessGranted) {
                m_attemptingNotesAccess = true;
                m_actualPasswordInput.clear();
                string asterisks(m_actualPasswordInput.length(), '*');
                m_passwordInputDisplay.setString(asterisks); // Clear visual input
                m_passwordMessageText.setString("Press Enter to submit, Esc to cancel.");
                m_statusLabel.setString("Notes Locked. Enter password. (Hint: The best instructor ever)"); // Updated Hint
                // Update nav item appearance for the "attempting access" state
                for (size_t i = 0; i < m_navItems.size(); ++i) {
                     m_navItems[i].background.setFillColor(i == index ? AppColors::NavActiveBG : Color::Transparent);
                     m_navItems[i].label.setFillColor(i == index ? AppColors::NavActiveText : AppColors::NavInactiveText);
                     m_navItems[i].label.setStyle(i == index ? Text::Bold : Text::Regular);
                }
                return; // Don't fully activate NotesWidget yet
            }
            // If access is already granted, fall through to normal activation
        } else {
            m_attemptingNotesAccess = false; // Not trying to access notes for other widgets
        }

        // Activate the new widget
        if (oldActiveWidgetIndex == -1 || !m_mainWidgets[index]->isActive() || oldActiveWidgetIndex != index) { // Activate if not already active or if it's a new widget
            m_mainWidgets[index]->activate();
        }


        // Update navigation item appearance
        for (size_t i = 0; i < m_navItems.size(); ++i) {
            if (i == m_activeWidgetIndex) {
                 m_navItems[i].background.setFillColor(AppColors::NavActiveBG);
                 m_navItems[i].label.setFillColor(AppColors::NavActiveText);
                 m_navItems[i].label.setStyle(Text::Bold);
            } else {
                 m_navItems[i].background.setFillColor(Color::Transparent);
                 m_navItems[i].label.setFillColor(AppColors::NavInactiveText);
                 m_navItems[i].label.setStyle(Text::Regular);
            }
        }

        // Update status label based on the newly active widget
        switch(m_activeWidgetIndex) {
            case 0: m_statusLabel.setString("Clock: Press 'F' to toggle 12/24 hr format."); break;
            case 1: m_statusLabel.setString("Weather (Mock): Press 'U' to update data."); break;
            case 2: // Notes widget - status already set if password was entered or being prompted
                    if(m_notesAccessGranted) m_statusLabel.setString("Notes: Click heading to select/edit. Scroll wheel. Add/Edit/Delete notes.");
                    // else: password prompt status is set above
                    break;
            case 3: m_statusLabel.setString("Calendar: Use < > buttons. Click day to see events."); break;
            case 4: m_statusLabel.setString("Alarm: Click input boxes or 'Set Alarm'. Use A/P for AM/PM."); break;
            case 5: m_statusLabel.setString("Stopwatch: Use buttons to control."); break;
            default: m_statusLabel.setString(""); break;
        }
    }

    RenderWindow m_window;
    RectangleShape m_navArea;
    RectangleShape m_widgetArea;
    RectangleShape m_statusArea;

    NoteManager m_noteManager;

    vector<unique_ptr<Widget>> m_mainWidgets;
    int m_activeWidgetIndex;

    struct NavItem {
        RectangleShape background;
        Text label;
    };
    vector<string> m_navItemsData;
    vector<NavItem> m_navItems;
    Text m_statusLabel;

    // Password related members
    Text m_passwordPromptText;
    Text m_passwordInputDisplay; // Shows asterisks
    Text m_passwordMessageText;  // For "Incorrect Password" or instructions
    string m_actualPasswordInput;
    bool m_notesAccessGranted;
    bool m_attemptingNotesAccess;
    const string NOTES_PASSWORD = "missnazia"; // Updated Password

    // Background Image members
    Texture m_backgroundTexture;
    Sprite m_backgroundSprite;
};

// --- Main Function ---
int main() {
    try {
        App dashboardApp;
        dashboardApp.run();
    } catch (const runtime_error& e) {
         cerr << "Runtime Error: " << e.what() << endl;
        return 1;
     } catch (const exception& e) {
         cerr << "Unhandled Exception: " << e.what() << endl;
         return 1;
    } catch (...) {
         cerr << "Unknown unhandled exception occurred!" << endl;
        return 1;
     }
    return 0;
}
// Example compilation:
// g++ main.cpp -o dashboard_app.exe -I/path/to/sfml/include -L/path/to/sfml/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
// Remember to copy SFML DLLs to the executable directory if linking dynamically.
// And ensure "background.png" and "arial.ttf" are present.+
//g++ -o test_program.exe main.cpp -ID:\SFML2\include -LD:\SFML2\lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio