#include "common.h"
#include "Observer.h"

Observer::Observer(Observable *o)
{
    APR_LOGD("Observer::Observer()");
    m_observable = o;
}

Observer::~Observer()
{
    APR_LOGD("Observer::~Observer()");
}

