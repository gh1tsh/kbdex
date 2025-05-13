#include "Decoder.hpp"

const std::string
Decoder::getKeycodeReprRu(int keycode)
{
        return Decoder::keycodesToRuReprTable.at(keycode);
}

const std::string
Decoder::getKeycodeReprEn(int keycode)
{
        return Decoder::kecodesToEnReprTable.at(keycode);
}

const std::string
Decoder::getKeycodeRepr(int keycode, Language lang)
{
        std::string repr;

        switch (lang)
        {
        case Language::RU:
                repr = Decoder::getKeycodeReprRu(keycode);
                break;
        case Language::EN:
                repr = Decoder::getKeycodeReprEn(keycode);
                break;
        default:
                repr = "UnknownKey";
        };

        return repr;
}

bool
Decoder::isLetter(int keycode)
{
        bool result;

        if ((keycode >= 16 && keycode <= 27) ||
            (keycode >= 30 && keycode <= 40) ||
            (keycode >= 44 && keycode <= 52)) {
                result = true;
        } else {
                result = false;
        }

        return result;
}

bool
Decoder::isDigit(int keycode)
{
        bool result;

        if (keycode >= 2 && keycode <= 11) {
                result = true;
        } else {
                result = false;
        }

        return result;
}

bool
Decoder::isWhitespace(int keycode) const
{
        bool result;

        if (keycode = 15 || keycode = 28 || keycode = 57) {
                result = true;
        } else {
                result = false;
        }

        return result;
}
