/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */

// SDL's PS5 video driver calls the hardware keyboard and the IME dialog
// while creating the window. A big-app sandbox does not map those two
// libraries, so the imports stay null and the first call faults at address 0.
// The DualSense path does not use them. These stand-ins keep that init alive.

extern "C"
{

    int sceKeyboardInit(void)
    {
        return -1;
    }

    int sceKeyboardOpen(int, int, int, void *)
    {
        return -1;
    }

    int sceKeyboardClose(int)
    {
        return 0;
    }

    int sceKeyboardReadState(int, void *)
    {
        return -1;
    }

    int sceImeDialogInit(const void *, void *)
    {
        return -1;
    }

    int sceImeDialogGetStatus(void)
    {
        return 0;
    }

    int sceImeDialogGetResult(void *)
    {
        return -1;
    }

    int sceImeDialogTerm(void)
    {
        return 0;
    }
}
