#include "radioOption.h"

void RadioOption::setSelectedOption(int _selected, bool callOnSelect)
{
    if (this->selected == _selected)
    {
        return;
    }
    if (_selected < 0)
    {
        log_i("ERROR OPTION INDEX NOT FOUND");
        return;
    }
    this->selected = _selected;
    for (int i = 0; i < this->size; i++)
    {
        this->options[i]->setBooleanValue(i == this->selected);
    }
    if (callOnSelect)
    {
        this->onSelect((void *)selected);
    }
}

int RadioOption::getOptionIndex(Option *option)
{
    for (int i = 0; i < this->size; i++)
    {
        if (this->options[i] == option)
        {
            return i;
        }
    }
    return -1;
}
void radioCB(void *data)
{
    Option *option = (Option *)data;
    RadioOption *radioOption = (RadioOption *)option->extraEventClickData;
    radioOption->setSelectedOption(radioOption->getOptionIndex(option), true);
}

RadioOption::RadioOption(lv_obj_t *parent, const char **text, int _size, option_event_cb_t _event_cb, int _selected)
{
    this->selected = -1;
    this->size = _size;
    this->options = (Option **)malloc(sizeof(Option *) * size);

    for (int i = 0; i < this->size; i++)
    {
        this->options[i] = new Option(parent, OptionType::RADIO, text[i], VALUE_ZERO, radioCB, this);
    }

    this->onSelect = _event_cb;

    // call last
    this->setSelectedOption(_selected);
}