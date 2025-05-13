#include "Decoder.hpp"

const std::string
Decoder::getKeycodeReprRu(int keycode) const
{
        return Decoder::keycodesToRuReprTable.at(keycode);
}

const std::string
Decoder::getKeycodeReprEn(int keycode) const
{
        return Decoder::kecodesToEnReprTable.at(keycode);
}

const std::string
Decoder::getKeycodeRepr(int keycode, Language lang) const
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
