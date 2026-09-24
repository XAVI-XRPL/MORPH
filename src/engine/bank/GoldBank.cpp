#include "ProgressionBank.h"

namespace morph
{

const BankEntry goldBank[] =
{
#include "GoldBank.inc"
};

const int goldBankCount = (int) (sizeof (goldBank) / sizeof (BankEntry));

const BankEntry* bankEntriesForStyle (StyleId style, int& count)
{
    // Bank is grouped by style; find the run.
    count = 0;
    const BankEntry* first = nullptr;
    for (int i = 0; i < goldBankCount; ++i)
    {
        if (goldBank[i].style == style)
        {
            if (first == nullptr)
                first = &goldBank[i];
            ++count;
        }
        else if (first != nullptr)
        {
            break;
        }
    }
    return first;
}

} // namespace morph
