#include "processor.h"

bool isAlert(const Metrics &m) {
    return (m.cpu > 80 || m.memory > 80);
}
