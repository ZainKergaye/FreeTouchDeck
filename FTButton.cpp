#include "FTButton.h"

static const char *module = "FTButton";
namespace FreeTouchDeck
{

    const char *enum_to_string(ButtonTypes type)
    {
        switch (type)
        {
            ENUM_TO_STRING_HELPER(ButtonTypes, STANDARD);
            ENUM_TO_STRING_HELPER(ButtonTypes, LATCH);
            ENUM_TO_STRING_HELPER(ButtonTypes, MENU);
        default:
            return "Unknown button type";
        }
    }
    const char *FTButton::JsonLabelLatchedLogo = "latchedlogo";
    const char *FTButton::JsonLabelLogo = "logo";
    const char *FTButton::JsonLabelType = "type";
    const char *FTButton::JsonLabelLabel = "label";
    const char *FTButton::JsonLabelActions = "actions";
    const char *FTButton::JsonLabelOutline = "outline";
    const char *FTButton::JsonLabelBackground = "background";
    const char *FTButton::JsonLabelTextColor = "textcolor";
    const char *FTButton::JsonLabelTextSize = "textsize";
    const char *FTButton::homeButtonTemplate = R"(
{ "logo":	"home.bmp",
    "type":	"STANDARD",
    "actions":	[{
            "type":	"MENU",
            "symbol":	"home"
        }]
    }    
)";
    const char *FTButton::backButtonTemplate = R"(			{
				"label": "Back",
				"logo": "arrow_back.bmp",
				"latchedlogo": "",
				"type": "STANDARD",
				"actions": [
					{
						"type": "MENU",
						"symbol": "~BACK"
					}
				]
			})";
    FTButton FTButton::BackButton(FTButton::backButtonTemplate, true);
    FTButton FTButton::HomeButton(FTButton::homeButtonTemplate, true);
    FTButton::FTButton(const char *templ, bool isShared)
    {
        IsShared = isShared;
        cJSON *doc = cJSON_Parse(templ);
        if (!doc)
        {
            const char *error = cJSON_GetErrorPtr();
            drawErrorMessage(true, module, "Unable to parse json string : %s", error);
        }
        else
        {
            Init(doc);
        }
        cJSON_Delete(doc);
    }
    ImageWrapper *FTButton::LatchedLogo()
    {
        ImageWrapper *image = NULL;
        if (_jsonLatchedLogo && strlen(_jsonLatchedLogo) > 0)
        {
            LOC_LOGV(module, "Latched Logo file name is %s", _jsonLatchedLogo);
            image = GetImage(_jsonLatchedLogo);
            if (image && !image->valid)
            {
                LOC_LOGD(module, "Latched Logo file %s is invalid.", _jsonLatchedLogo);
                image = NULL;
            }
        }
        return image;
    }
    bool FTButton::IsLabelDraw()
    {
        ImageWrapper *image = NULL;
        if (!ISNULLSTRING(_jsonLogo))
        {
            image = GetImage(_jsonLogo);
        }
        else
        {
            LOC_LOGD(module, "Empty logo value");
        }
        if (!image || !image->valid)
        {
            return true;
        }
        return false;
    }
    ImageWrapper *FTButton::Logo()
    {
        ImageWrapper *image = NULL;

        if (_jsonLogo && strlen(_jsonLogo) > 0)
        {
            LOC_LOGV(module, "Logo file name is %s", _jsonLogo);
            image = GetImage(_jsonLogo);
        }
        else
        {
            LOC_LOGD(module, "empty logo file name");
        }

        if (!image || !image->valid)
        {
            LOC_LOGD(module, "Logo file name %s was not found. Defaulting to label.bmp", _jsonLogo ? _jsonLogo : "");
            image = GetImage("label.bmp");
        }
        return image;
    }
    bool FTButton::Latch(FTAction *action)
    {
        LOC_LOGD(module, "Button is Executing Action %s", action->toString());
        if (ButtonType == ButtonTypes::LATCH && action->IsLatch())
        {
            switch (action->Type)
            {
            case ActionTypes::SETLATCH:
                if (!Latched)
                {
                    Latched = true;
                    Invalidate();
                }
                break;
            case ActionTypes::CLEARLATCH:
                if (Latched)
                {
                    Latched = false;
                    Invalidate();
                }
                break;
            case ActionTypes::TOGGLELATCH:
                Latched = !Latched;
                Invalidate();
                break;
            default:
                break;
            }
        }
        else
        {
            LOC_LOGW(module, "Cannot latch button. Button %s has type %s.", STRING_OR_DEFAULT(Label, _jsonLogo), enum_to_string(ButtonType));
            return false;
        }
        return true;
    }

    void FTButton::SetCoordinates(uint16_t width, uint16_t height, uint16_t row, uint16_t col, uint8_t spacing)
    {
        uint16_t centerX = 0;
        uint16_t centerY = 0;
        ButtonWidth = width;
        ButtonHeight = height;
        Spacing = spacing;
        centerX = (col * 2 + 1) * ButtonWidth / 2 + col * spacing;
        centerY = (row * 2 + 1) * ButtonHeight / 2 + row * spacing;
        if (width != ButtonWidth || ButtonHeight != height || centerX != CenterX || centerY != CenterY || Spacing != spacing)
        {
            LOC_LOGD(module, "Button size %dx%d, row %d, col %d  ", width, height, row, col);
            CenterX = centerX;
            CenterY = centerY;
        }
    }
    FTButton::FTButton(const char *label, uint8_t index, cJSON *button, uint16_t outline, uint8_t textSize, uint16_t textColor) : Outline(outline), TextSize(textSize), TextColor(textColor)
    {
        char menuName[31] = {0};
        LOC_LOGD(module, "Button Constructor for menu button");
        if (button && cJSON_IsString(button))
        {
            _jsonLogo = strdup(cJSON_GetStringValue(button));
        }
        else
        {
            LOC_LOGW(module, "Menu button does not have a logo!");
        }
        cJSON *jsonActionValue = NULL;
        ButtonType = ButtonTypes::MENU;
        BackgroundColor = generalconfig.menuButtonColour;
        snprintf(menuName, sizeof(menuName), "menu%d", index + 1);
        Label = ps_strdup(STRING_OR_DEFAULT(label, ""));
        LOC_LOGD(module, "New button name is %s, menu name is %s", Label, menuName);
        actions.push_back(new FTAction(ActionTypes::MENU, menuName));
    }
    FTButton::FTButton(const char *label, uint8_t index, cJSON *document, cJSON *button, uint16_t outline, uint8_t textSize, uint16_t textColor) : Outline(outline), TextSize(textSize), TextColor(textColor)
    {
        char logoName[31] = {0};
        cJSON *jsonActionValue = NULL;
        cJSON *_jsonLatch = NULL;
        LOC_LOGD(module, "Button Constructor for action button");
        snprintf(logoName, sizeof(logoName), "logo%d", index);
        cJSON *jsonLogo = cJSON_GetObjectItem(document, logoName);
        if (jsonLogo && cJSON_IsString(jsonLogo))
        {
            _jsonLogo = strdup(cJSON_GetStringValue(jsonLogo));
        }
        else
        {
            LOC_LOGW(module, "No logo file was found for %s", jsonLogo);
        }
        Label = ps_strdup(STRING_OR_DEFAULT(label, ""));
        cJSON *jsonLatchedLogo = cJSON_GetObjectItem(button, "latchlogo");
        if (jsonLatchedLogo && cJSON_IsString(jsonLatchedLogo))
        {
            _jsonLatchedLogo = strdup(cJSON_GetStringValue(jsonLatchedLogo));
        }
        _jsonLatch = cJSON_GetObjectItem(button, "latch");
        ButtonType = (_jsonLatch && cJSON_IsBool(_jsonLatch) && cJSON_IsTrue(_jsonLatch)) ? ButtonTypes::LATCH : ButtonTypes::STANDARD;

        if (ButtonType != ButtonTypes::MENU)
        {
            BackgroundColor = generalconfig.functionButtonColour;
        }
        else
        {
            BackgroundColor = generalconfig.menuButtonColour;
        }
        cJSON *jsonActions = cJSON_GetObjectItem(button, "actionarray");

        if (!jsonActions)
        {
            LOC_LOGE(module, "Button %s does not have any action!", Logo()->LogoName);
        }
        else
        {
            uint8_t valuePos = 0;
            LOC_LOGD(module, "Button %s has %d actions", Logo()->LogoName, cJSON_GetArraySize(jsonActions));
            cJSON_ArrayForEach(jsonActionValue, cJSON_GetObjectItem(button, "valuearray"))
            {
                cJSON *jsonAction = cJSON_GetArrayItem(jsonActions, valuePos++);
                if (!jsonAction)
                {
                    LOC_LOGE(module, "Current value does not have a matching action!");
                }
                else if (FTAction::GetType(jsonAction) != ActionTypes::NONE)
                {
                    LOC_LOGD(module, "Adding action to button %s, type %s", Logo()->LogoName, enum_to_string(FTAction::GetType(jsonAction)));
                    auto action = new FTAction(jsonAction, jsonActionValue);
                    if (!action)
                    {
                        LOC_LOGE(module, "Could not allocate memory for action");
                    }
                    else
                    {

                        actions.push_back(action);
                        LOC_LOGD(module, "DONE Adding action to button %s, type %s", Logo()->LogoName, enum_to_string(FTAction::GetType(jsonAction)));
                    }

                    // only push valid actions
                    PrintMemInfo();
                }
                else
                {
                    LOC_LOGD(module, "Ignoring action type NONE for button %s", Logo()->LogoName);
                }
            }
        }
    }

    FTButton::~FTButton()
    {
        FREE_AND_NULL(Label);
        FREE_AND_NULL(_jsonLogo);
        FREE_AND_NULL(_jsonLatchedLogo);
    }
    void FTButton::Invalidate()
    {
        NeedsDraw = true;
    }
    ImageWrapper *FTButton::GetActiveImage()
    {
        ImageWrapper *image = NULL;
        if (Latched)
        {
            image = LatchedLogo();
        }
        if (!image)
        {
            image = Logo();
        }
        return image;
    }
    void FTButton::Draw(bool force)
    {
        bool transparent = false;
        int32_t radius = 4;
        uint16_t BGColor = TFT_BLACK;
        uint16_t adjustedWidth = ButtonWidth - (2 * Spacing);
        uint16_t adjustedHeight = ButtonHeight - (2 * Spacing);

        if (!NeedsDraw && !force)
            return;

        NeedsDraw = false;
        ImageWrapper *image = GetActiveImage();
        if (!image || !image->valid)
        {
            LOC_LOGE(module, "No image found, or image invalid!");
        }
        else
        {
            BGColor = tft.color565(image->R, image->G, image->B);
            BackgroundColor = BGColor;
        }
        LOC_LOGV(module, "Found image structure, bitmap is %s", image->LogoName);
        LOC_LOGD(module, "Drawing button at [%d,%d] size: %dx%d,  outline : 0x%04X, BG Color: 0x%04X, Text color: 0x%04X, Text size: %d", CenterX, CenterY, adjustedWidth, adjustedHeight, Outline, BGColor, TextColor, TextSize);
        PrintMemInfo();

        _button.initButton(&tft, CenterX, CenterY, adjustedWidth, adjustedHeight, Outline, BackgroundColor, TextColor, (char *)(IsLabelDraw() ? Label : ""), TextSize);
        _button.drawButton();
        bool LatchNeedsRoundRect = !LatchedLogo();

        tft.setFreeFont(LABEL_FONT);

        if (ButtonType == ButtonTypes::LATCH && LatchNeedsRoundRect)
        {
            uint32_t roundRectWidth = ButtonWidth / 4;
            uint32_t roundRectHeight = ButtonHeight / 4;
            uint32_t cornerX = CenterX - (adjustedWidth / 2) + roundRectWidth / 2;
            uint32_t cornerY = CenterY - (adjustedHeight / 2) + roundRectHeight / 2;
            if (Latched)
            {
                LOC_LOGD(module, "Latched without a latched logo.  Drawing round rectangle");
                tft.fillRoundRect(cornerX, cornerY, roundRectWidth, roundRectHeight, radius, generalconfig.latchedColour);
                transparent = true;
            }
            else
            {
                LOC_LOGD(module, "Latched deactivated without a latched logo.  Erasing round rectangle");
                tft.fillRoundRect(cornerX, cornerY, roundRectWidth, roundRectHeight, radius, BackgroundColor);
                // draw button one more time
                _button.drawButton();
            }
        }
        if (image && image->valid)
        {
            image->Draw(CenterX, CenterY, transparent);
        }
    }
    uint16_t FTButton::Width()
    {
        return LatchedLogo() ? max(Logo()->w, LatchedLogo()->w) : Logo()->w;
    }
    uint16_t FTButton::Height()
    {
        return LatchedLogo() ? max(Logo()->h, LatchedLogo()->h) : Logo()->h;
    }

    void FTButton::Press()
    {
        bool needsRelease = false;
        if (IsPressed)
        {
            LOC_LOGV(module, "Button already pressed. Ignoring");
            return;
        }
        // Beep
        HandleAudio(Sounds::BEEP);
        LOC_LOGD(module, "%s Button Press detected with %d actions", enum_to_string(ButtonType), actions.size());
        for (FTAction *action : actions)
        {
            needsRelease = action->NeedsRelease ? true : needsRelease;
            LOC_LOGD(module, "Queuing action %s", enum_to_string(action->Type), action->toString());
            if (!QueueAction(action))
            {
                LOC_LOGW(module, "Button action %s could not be queued for execution.", action->toString());
            }
        }
        if (ButtonType == ButtonTypes::LATCH)
        {
            Latched = !Latched;
            LOC_LOGD(module, "Toggling LATCH to %s", Latched ? "ACTIVE" : "INACTIVE");
        }
        IsPressed = true;
        NeedsDraw = true;
        if (needsRelease)
        {
            QueueAction(&FTAction::releaseAllAction);
        }
    }
    void FTButton::Release()
    {
        if (IsPressed)
        {
            LOC_LOGD(module, "Releasing button with logo %s", Logo()->LogoName);
            IsPressed = false;
            NeedsDraw = true;
        }
    }
    cJSON *FTButton::ToJSON()
    {
        cJSON *button = cJSON_CreateObject();
        if (!button)
        {
            drawErrorMessage(true, module, "Memory allocation failed when rendering JSON button");
            return NULL;
        }
        LOC_LOGD(module, "Adding button members to Json");
        if (!ISNULLSTRING(Label))
        {
            cJSON_AddStringToObject(button, FTButton::JsonLabelLabel, STRING_OR_DEFAULT(Label, ""));
        }
        if (!ISNULLSTRING(_jsonLogo))
        {
            cJSON_AddStringToObject(button, FTButton::JsonLabelLogo, _jsonLogo ? _jsonLogo : "");
        }
        if (ButtonType == ButtonTypes::LATCH && !ISNULLSTRING(_jsonLatchedLogo))
        {
            cJSON_AddStringToObject(button, FTButton::JsonLabelLatchedLogo, _jsonLatchedLogo ? _jsonLatchedLogo : "");
        }
        cJSON_AddStringToObject(button, FTButton::JsonLabelType, enum_to_string(ButtonType));

        if (Outline != generalconfig.DefaultOutline)
        {
            cJSON_AddNumberToObject(button, FTButton::JsonLabelOutline, Outline);
        }
        if (BackgroundColor != generalconfig.backgroundColour)
        {
            cJSON_AddNumberToObject(button, FTButton::JsonLabelBackground, BackgroundColor);
        }
        if (TextColor != generalconfig.DefaultTextColor)
        {
            cJSON_AddNumberToObject(button, FTButton::JsonLabelTextColor, TextColor);
        }
        if (TextSize != generalconfig.DefaultTextSize)
        {
            cJSON_AddNumberToObject(button, FTButton::JsonLabelTextSize, TextSize);
        }
        LOC_LOGD(module, "Adding actions to Json");
        if (actions.size() > 0)
        {
            cJSON *actionsJson = cJSON_CreateArray();
            for (auto action : actions)
            {
                if (action->Type != ActionTypes::NONE)
                {
                    cJSON_AddItemToArray(actionsJson, action->ToJson());
                }
            }
            cJSON_AddItemToObject(button, FTButton::JsonLabelActions, actionsJson);
        }
        if (generalconfig.LogLevel >= LogLevels::VERBOSE)
        {
            char *buttonString = cJSON_Print(button);
            if (buttonString)
            {
                LOC_LOGD(module, "Button json structure : \n%s", buttonString);
                FREE_AND_NULL(buttonString);
            }
            else
            {
                LOC_LOGE(module, "Unable to format JSON for output!");
            }
        }
        return button;
    }
    bool FTButton::contains(uint16_t x, uint16_t y)
    {
        return _button.contains(x, y);
    }
    void FTButton::Init(cJSON *button)
    {
        char *buttonType = NULL;
        GetValueOrDefault(button, FTButton::JsonLabelType, &buttonType, enum_to_string(ButtonTypes::STANDARD));
        ButtonType = parse_button_types(buttonType);
        FREE_AND_NULL(buttonType);
        LOC_LOGD(module, "Button type is %s", enum_to_string(ButtonType));

        GetValueOrDefault(button, FTButton::JsonLabelLabel, &Label, "");
        LOC_LOGD(module, "Label: %s", Label);
        GetValueOrDefault(button, FTButton::JsonLabelLogo, &_jsonLogo, "");
        LOC_LOGD(module, "Logo : %s", _jsonLogo);
        if (ButtonType == ButtonTypes::LATCH)
        {
            GetValueOrDefault(button, FTButton::JsonLabelLatchedLogo, &_jsonLatchedLogo, "");
            LOC_LOGD(module, "Latched logo: %s", _jsonLatchedLogo);
        }

        GetValueOrDefault(button, FTButton::JsonLabelOutline, &Outline, generalconfig.DefaultOutline);
        GetValueOrDefault(button, FTButton::JsonLabelBackground, &BackgroundColor, generalconfig.backgroundColour);
        GetValueOrDefault(button, FTButton::JsonLabelTextColor, &TextColor, generalconfig.DefaultTextColor);
        GetValueOrDefault(button, FTButton::JsonLabelTextSize, &TextSize, generalconfig.DefaultTextSize);

        cJSON *jsonActions = cJSON_GetObjectItem(button, FTButton::JsonLabelActions);
        if (!jsonActions)
        {
            LOC_LOGE(module, "Button %s does not have any action!", Logo()->LogoName);
        }
        else
        {
            if (!cJSON_IsArray(jsonActions))
            {
                LOC_LOGE(module, "Actions object for button %s should be an array but it is not.", Label);
            }
            cJSON *actionJson = NULL;
            cJSON_ArrayForEach(actionJson, jsonActions)
            {
                auto action = new FTAction(actionJson);
                if (action->Type != ActionTypes::NONE)
                {
                    actions.push_back(action);
                }
            }
        }
    }
    FTButton::FTButton(cJSON *button)
    {
        Init(button);
    }
    ButtonTypes &operator++(ButtonTypes &state, int)
    {
        int i = static_cast<int>(state) + 1;
        i = i >= (int)ButtonTypes::ENDLIST ? (int)ButtonTypes::NONE : i;
        state = static_cast<ButtonTypes>(i);
        return state;
    }
    ButtonTypes parse_button_types(const char *buttonType)
    {
        ButtonTypes Result = ButtonTypes::NONE;
        do
        {
            Result++; // Start after NONE
        } while (strcmp(buttonType, enum_to_string(Result)) != 0 && Result != ButtonTypes::NONE);
        return Result;
    }
};
