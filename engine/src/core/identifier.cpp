#include "identifier.h"
#include <time.h>
#include "containers/darray.h"
#include "math/mtrand.h"

// static bool generator_created = false;
static MtRand generator;

Identifier& Identifier::Generate()
{
    if (!generator) {
        generator = MtRand(time(0));
        // generator_created = true;
    }
    
    UniqueID = generator.Generate();
    return *this;
}
