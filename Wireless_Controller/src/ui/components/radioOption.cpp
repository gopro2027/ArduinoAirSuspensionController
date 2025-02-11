#include "radioOption.h"

template <size_t N>
void RadioOption<N>::setSelectedOption(int _selected)
{
    this->selected = _selected;
}
template <size_t N>
RadioOption<N>::RadioOption(lv_obj_t *parent, const char **text, option_event_cb_t _event_cb, int _selected)
{

    // call last
    this->setSelectedOption(_selected);
}