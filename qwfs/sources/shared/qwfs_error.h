#pragma once
#include <quiche.h>

#include "qwfs_types.h"

namespace qwfs
{
	QwfsStatus ConvertQuicheH3ErrorToStatus(quiche_h3_error error, QwfsStatus nowStatus);
}
