/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "ui.h"

#include "app.h"
#include "core.h"
#include "font.h"
#include "input.h"
#include "patch.h"
#include "saves.h"
#include "trace.h"
#include "util.h"

#include <libretro.h>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

extern "C" void bigalloc_stats(size_t *mapped, unsigned *count);

namespace
{
static_assert(RETROK_F12 == RETROK_F1 + 11, "F-keys must be contiguous");

const SDL_Color kBg{8, 46, 18, 255};
const SDL_Color kShade{0, 0, 0, 160};
const SDL_Color kText{236, 236, 220, 255};
const SDL_Color kHi{255, 214, 102, 255};
const SDL_Color kDim{176, 188, 168, 255};
const SDL_Color kBar{24, 92, 36, 255};

const char *kFormLabel[] = {"3-3-4", "3-4-3", "4-2-4", "4-3-3", "4-4-2", "4-5-1",
                            "5-2-3", "5-3-2", "5-4-1", "5-5-0", "6-3-1", "6-4-0"};

const char *kFaq =
    "Controles\n"
    "O direcional e o analogico esquerdo movem o cursor do jogo. Cruz confirma. Circulo e Esc.\n"
    "Quadrado envia A, triangulo envia M, L1 envia asterisco e R1 envia menos.\n"
    "O analogico direito move o mouse. R2 e o clique esquerdo e L2 e o clique direito.\n"
    "Options abre este menu. Create abre o teclado.\n\n"
    "Jogos salvos\n"
    "Os saves em D:\\ELI98\\JOGOS ficam na pasta do jogo, no console ou no pendrive. "
    "Exporte uma copia para o USB pelo menu quando quiser um backup.\n\n"
    "Patches\n"
    "Coloque o zip do patch na raiz do pendrive. Ele precisa ter pelo menos 34 arquivos .EFT "
    "quando trocar as equipes. "
    "O jogo reinicia e o PATCH.BAT aplica times e bandeiras.\n\n"
    "Editar equipes\n"
    "Saia do jogo ate o Windows, abra o File Manager em D:\\ELI98 e rode editeq.exe.\n\n"
    "Acentos\n"
    "O teclado cobre letras, numeros e pontuacao. O Elifoot 98 nem sempre aceita acentos.\n\n"
    "Este projeto e gratuito, feito por fas, e nao coleta dados.";

const char *kAbout =
    "Elifoot 98 no PS5\n"
    "Porta homebrew do Elifoot 98 Online. O jogo roda no DOSBox Pure, com o mesmo disco de Windows "
    "3.1 da versao de navegador.\n\n"
    "Elifoot 98 e um jogo de Andre Elias e Ricardo Correia, de 1998, distribuido como shareware. "
    "A comercializacao sem autorizacao dos autores e proibida.\n\n"
    "A versao de navegador e de Leonardo \"Leu\" Pereira, licenciada em MIT. "
    "Este titulo e GPL-3.0 porque junta o DOSBox Pure ao sistema de build.\n\n"
    "Codigo do menu, dos saves e do Cheat 'O Matic: Elifoot 98 Online.";

struct KeyCell
{
    const char *label;
    unsigned key;
    bool letter;
    int action;
};

enum
{
    ActChar = 0,
    ActSpace,
    ActBack,
    ActEnter,
    ActShift,
    ActClose
};

const KeyCell kRow0[] = {{"1", RETROK_1, false, ActChar}, {"2", RETROK_2, false, ActChar},
                         {"3", RETROK_3, false, ActChar}, {"4", RETROK_4, false, ActChar},
                         {"5", RETROK_5, false, ActChar}, {"6", RETROK_6, false, ActChar},
                         {"7", RETROK_7, false, ActChar}, {"8", RETROK_8, false, ActChar},
                         {"9", RETROK_9, false, ActChar}, {"0", RETROK_0, false, ActChar}};
const KeyCell kRow1[] = {{"Q", RETROK_q, true, ActChar}, {"W", RETROK_w, true, ActChar},
                         {"E", RETROK_e, true, ActChar}, {"R", RETROK_r, true, ActChar},
                         {"T", RETROK_t, true, ActChar}, {"Y", RETROK_y, true, ActChar},
                         {"U", RETROK_u, true, ActChar}, {"I", RETROK_i, true, ActChar},
                         {"O", RETROK_o, true, ActChar}, {"P", RETROK_p, true, ActChar}};
const KeyCell kRow2[] = {
    {"A", RETROK_a, true, ActChar}, {"S", RETROK_s, true, ActChar}, {"D", RETROK_d, true, ActChar},
    {"F", RETROK_f, true, ActChar}, {"G", RETROK_g, true, ActChar}, {"H", RETROK_h, true, ActChar},
    {"J", RETROK_j, true, ActChar}, {"K", RETROK_k, true, ActChar}, {"L", RETROK_l, true, ActChar}};
const KeyCell kRow3[] = {{"Z", RETROK_z, true, ActChar},     {"X", RETROK_x, true, ActChar},
                         {"C", RETROK_c, true, ActChar},     {"V", RETROK_v, true, ActChar},
                         {"B", RETROK_b, true, ActChar},     {"N", RETROK_n, true, ActChar},
                         {"M", RETROK_m, true, ActChar},     {".", RETROK_PERIOD, false, ActChar},
                         {"-", RETROK_MINUS, false, ActChar}};
const KeyCell kRow4[] = {{"SHIFT", 0, false, ActShift},
                         {"ESPACO", RETROK_SPACE, false, ActSpace},
                         {"APAGAR", RETROK_BACKSPACE, false, ActBack},
                         {"ENTER", RETROK_RETURN, false, ActEnter},
                         {"FECHAR", 0, false, ActClose}};

const KeyCell *kRows[] = {kRow0, kRow1, kRow2, kRow3, kRow4};
const int kRowLen[] = {10, 10, 9, 9, 5};

int ui_scale()
{
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(app().renderer, &w, &h);
    if (h >= 900)
        return 3;
    return 2;
}

void fill(SDL_Color color)
{
    SDL_SetRenderDrawColor(app().renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(app().renderer);
}

void present()
{
    // SDL_CreateSoftwareRenderer flushes drawing into this surface, but it
    // has no window attached and therefore cannot flip the PS5 framebuffer.
    SDL_RenderPresent(app().renderer);
    if (app().framebuffer && SDL_UpdateWindowSurface(app().window) != 0)
    {
        static bool reported = false;
        if (!reported)
        {
            trace("SDL_UpdateWindowSurface falhou: %s", SDL_GetError());
            reported = true;
        }
    }
}

void shade()
{
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(app().renderer, &w, &h);
    SDL_SetRenderDrawColor(app().renderer, kShade.r, kShade.g, kShade.b, kShade.a);
    SDL_Rect rect{0, 0, w, h};
    SDL_RenderFillRect(app().renderer, &rect);
}

void open_screen(Screen screen)
{
    app().screen = screen;
    app().cursor = 0;
    app().scroll = 0;
}

std::vector<std::string> menu_items()
{
    std::vector<std::string> items;
    items.push_back("Formações");
    items.push_back("Digitar texto");
    items.push_back(std::string("Suavização: ") + (app().smooth ? "ligada" : "desligada"));
    items.push_back("Exportar jogos salvos");
    items.push_back("Importar jogo salvo");
    items.push_back("Carregar patch");
    items.push_back("Remover patch");
    items.push_back("Apagar todos os dados");
    items.push_back("Cheat 'O Matic");
    items.push_back("FAQ");
    items.push_back("Sobre");
    items.push_back("Reiniciar jogo");
    items.push_back("Sair");
    return items;
}

void move_cursor(int delta, int count)
{
    if (count <= 0)
        return;
    int next = app().cursor + delta;
    if (next < 0)
        next = count - 1;
    if (next >= count)
        next = 0;
    app().cursor = next;
    if (app().cursor < app().scroll)
        app().scroll = app().cursor;
    if (app().cursor >= app().scroll + 12)
        app().scroll = app().cursor - 11;
}

void open_keyboard(EditTarget target)
{
    app().edit = target;
    app().shift = false;
    app().key_row = 0;
    app().key_col = 0;
    if (target == EditTarget::CheatName)
        app().cheat_name.clear();
    open_screen(Screen::Keyboard);
}

void open_files(FileKind kind)
{
    app().file_kind = kind;
    app().files = kind == FileKind::Save ? find_import_saves() : find_patch_zips();
    open_screen(Screen::Files);
}

void open_confirm(ConfirmKind kind, const std::string &text)
{
    app().confirm = kind;
    app().confirm_text = text;
    open_screen(Screen::Confirm);
}

void open_text(const char *title, const char *body)
{
    app().text_title = title;
    app().text_body = body;
    app().text_scroll = 0;
    open_screen(Screen::Text);
}

std::string *edit_buffer()
{
    if (app().edit == EditTarget::Omatic)
        return &app().omatic.input;
    if (app().edit == EditTarget::CheatName)
        return &app().cheat_name;
    return nullptr;
}

void close_keyboard()
{
    if (app().edit == EditTarget::CheatName)
    {
        std::string error = app().omatic.save_cheat(app().cheat_name);
        if (!error.empty())
            app_status(error);
        else
            app_status("Cheat salvo.");
        open_screen(Screen::Omatic);
        return;
    }
    if (app().edit == EditTarget::Omatic)
    {
        open_screen(Screen::Omatic);
        return;
    }
    open_screen(Screen::Playing);
}

void type_key(const KeyCell &cell)
{
    std::string *buffer = edit_buffer();
    if (cell.action == ActShift)
    {
        app().shift = !app().shift;
        return;
    }
    if (cell.action == ActClose)
    {
        close_keyboard();
        return;
    }
    if (buffer)
    {
        if (cell.action == ActBack)
        {
            if (!buffer->empty())
                buffer->pop_back();
        }
        else if (cell.action == ActSpace)
        {
            buffer->push_back(' ');
        }
        else if (cell.action == ActEnter)
        {
            close_keyboard();
        }
        else if (cell.letter)
        {
            char ch = cell.label[0];
            if (!app().shift && ch >= 'A' && ch <= 'Z')
                ch = (char)(ch - 'A' + 'a');
            buffer->push_back(ch);
        }
        else if (cell.label && cell.label[0])
        {
            buffer->push_back(cell.label[0]);
        }
        return;
    }
    if (cell.action == ActSpace)
        input_pulse(RETROK_SPACE, false);
    else if (cell.action == ActBack)
        input_pulse(RETROK_BACKSPACE, false);
    else if (cell.action == ActEnter)
        input_pulse(RETROK_RETURN, false);
    else if (cell.key)
        input_pulse(cell.key, app().shift && cell.letter);
}

void accept_confirm()
{
    std::string message;
    switch (app().confirm)
    {
    case ConfirmKind::Wipe:
        message = wipe_data(app().data_dir);
        if (message.rfind("Nao foi", 0) == 0)
            app_status(message);
        else
            app_reload(message);
        break;
    case ConfirmKind::ClearPatch:
        message = clear_patch(app().data_dir);
        app_reload(message);
        break;
    case ConfirmKind::Quit:
        app().running = false;
        break;
    case ConfirmKind::Restart:
        app_reload("Jogo reiniciado.");
        break;
    case ConfirmKind::ApplyPatch:
        message = apply_patch(app().data_dir, app().pending_patch);
        if (!message.empty())
            app_status(message);
        else
            app_reload("Patch aplicado.");
        break;
    case ConfirmKind::ImportSave:
        message = import_save(app().data_dir, app().pending_import);
        if (message.rfind("Jogo salvo", 0) == 0)
            app_reload(message);
        else
            app_status(message);
        break;
    case ConfirmKind::None:
        break;
    }
}

void activate_menu()
{
    switch (app().cursor)
    {
    case 0:
        open_screen(Screen::Formations);
        break;
    case 1:
        open_keyboard(EditTarget::Game);
        break;
    case 2:
        app().smooth = !app().smooth;
        app_status(app().smooth ? "Suavização ligada." : "Suavização desligada.");
        break;
    case 3:
        app_status(export_saves(app().data_dir));
        break;
    case 4:
        open_files(FileKind::Save);
        break;
    case 5:
        open_files(FileKind::Patch);
        break;
    case 6:
        open_confirm(ConfirmKind::ClearPatch,
                     "Remover o patch? Times e bandeiras voltam ao original.");
        break;
    case 7:
        open_confirm(ConfirmKind::Wipe, "Apagar jogos salvos, patch e alterações do Windows?");
        break;
    case 8:
        app().omatic.refresh_labels();
        open_screen(Screen::Omatic);
        break;
    case 9:
        open_text("FAQ", kFaq);
        break;
    case 10:
        open_text("Sobre", kAbout);
        break;
    case 11:
        open_confirm(ConfirmKind::Restart,
                     "Reiniciar o jogo? O que não foi salvo dentro do Elifoot será perdido.");
        break;
    case 12:
        open_confirm(ConfirmKind::Quit, "Fechar o Elifoot 98?");
        break;
    default:
        break;
    }
}

std::vector<std::string> omatic_items()
{
    app().omatic.refresh_labels();
    std::vector<std::string> items;
    items.push_back(app().omatic.field_label + ": " + app().omatic.input);
    items.push_back(app().omatic.primary_label);
    items.push_back("Nova busca");
    items.push_back("Salvar código");
    items.push_back("Cheats salvos");
    items.push_back("Voltar");
    return items;
}

void activate_omatic()
{
    switch (app().cursor)
    {
    case 0:
        open_keyboard(EditTarget::Omatic);
        break;
    case 1:
        if (app().omatic.state == SearchState::Found)
        {
            std::string error = app().omatic.set_value();
            app_status(error.empty() ? "Valor alterado." : error);
        }
        else
        {
            std::string error = app().omatic.search();
            if (!error.empty())
                app_status(error);
        }
        app().omatic.refresh_labels();
        break;
    case 2:
        app().omatic.reset();
        break;
    case 3:
        if (app().omatic.state != SearchState::Found)
            app_status("Busque até restar um endereço.");
        else
            open_keyboard(EditTarget::CheatName);
        break;
    case 4:
        open_screen(Screen::Cheats);
        break;
    default:
        open_screen(Screen::Menu);
        break;
    }
}

void activate_file()
{
    if (app().cursor < 0 || app().cursor >= (int)app().files.size())
        return;
    const std::string &path = app().files[(size_t)app().cursor];
    if (app().file_kind == FileKind::Save)
    {
        app().pending_import = path;
        open_confirm(ConfirmKind::ImportSave,
                     "Se já existir um save com o mesmo nome, ele será substituído. Continuar?");
        return;
    }
    std::string error = prepare_patch(path, app().pending_patch);
    if (!error.empty())
    {
        app_status(error);
        return;
    }
    char buf[160];
    snprintf(buf, sizeof(buf),
             "%u arquivos do patch serão carregados, incluindo bandeiras, equipes e configuração. "
             "Continuar?",
             (unsigned)app().pending_patch.size());
    open_confirm(ConfirmKind::ApplyPatch, buf);
}

void draw_status(int scale)
{
    if (app().status_ttl <= 0 || app().status.empty())
        return;
    app().status_ttl--;
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(app().renderer, &w, &h);
    int y = h - font_height(scale) - 24;
    SDL_SetRenderDrawColor(app().renderer, 0, 0, 0, 180);
    SDL_Rect bar{0, y - 8, w, font_height(scale) + 20};
    SDL_RenderFillRect(app().renderer, &bar);
    font_draw(app().renderer, 24, y, app().status.c_str(), kHi, scale);
}

void draw_items(const char *title, const std::vector<std::string> &items, const char *hint)
{
    int scale = ui_scale();
    int line = font_height(scale) + scale * 2;
    int y = 36;
    font_draw(app().renderer, 48, y, title, kHi, scale);
    y += line * 2;
    int shown = 0;
    for (int i = app().scroll; i < (int)items.size() && shown < 12; i++, shown++)
    {
        if (i == app().cursor)
        {
            SDL_SetRenderDrawColor(app().renderer, kBar.r, kBar.g, kBar.b, 255);
            SDL_Rect bar{36, y - 4, 1200, line};
            SDL_RenderFillRect(app().renderer, &bar);
        }
        font_draw(app().renderer, 48, y, items[(size_t)i].c_str(), i == app().cursor ? kHi : kText,
                  scale);
        y += line;
    }
    int h = 0, w = 0;
    SDL_GetRendererOutputSize(app().renderer, &w, &h);
    font_draw(app().renderer, 48, h - line * 2, hint, kDim, scale > 2 ? scale - 1 : scale);
}

void draw_keyboard()
{
    int scale = ui_scale() > 2 ? 2 : ui_scale();
    std::string *buffer = edit_buffer();
    const char *title =
        buffer ? (app().edit == EditTarget::CheatName ? "Nome do cheat" : "Valor") : "Teclado";
    font_draw(app().renderer, 48, 28, title, kHi, scale + 1);
    if (buffer)
        font_draw(app().renderer, 48, 28 + font_height(scale + 1) + 8, buffer->c_str(), kText,
                  scale);
    int y = 140;
    for (int row = 0; row < 5; row++)
    {
        int x = 48;
        for (int col = 0; col < kRowLen[row]; col++)
        {
            const KeyCell &cell = kRows[row][col];
            const char *label = cell.label;
            std::string shifted;
            if (cell.letter && !app().shift)
            {
                shifted = label;
                if (!shifted.empty() && shifted[0] >= 'A' && shifted[0] <= 'Z')
                    shifted[0] = (char)(shifted[0] - 'A' + 'a');
                label = shifted.c_str();
            }
            if (cell.action == ActShift && app().shift)
                label = "SHIFT*";
            int width = font_width(label, scale) + 18;
            if (row == app().key_row && col == app().key_col)
            {
                SDL_SetRenderDrawColor(app().renderer, kBar.r, kBar.g, kBar.b, 255);
                SDL_Rect bar{x - 6, y - 4, width, font_height(scale) + 8};
                SDL_RenderFillRect(app().renderer, &bar);
            }
            font_draw(app().renderer, x, y, label, kText, scale);
            x += width + 10;
        }
        y += font_height(scale) + 16;
    }
}

bool seed_ready()
{
    const char *seed = "/app0/assets/elifoot98";
    return path_is_file(path_join(seed, "dosbox.conf")) &&
           path_is_file(path_join(seed, "hdd.img")) &&
           path_is_file(path_join(seed, "d/ELI98/ELIFOOT.EXE"));
}

bool locate_data(std::string &error)
{
    const char *seed = "/app0/assets/elifoot98";
    if (!seed_ready())
    {
        error = "O disco do jogo não está em /app0/assets/elifoot98.";
        return false;
    }
    trace("disco em %s", seed);
    std::string drive_d = path_join(seed, "d");
    DIR *listing = opendir(drive_d.c_str());
    if (!listing)
        trace("opendir da pasta D falhou: %s", strerror(errno));
    else
    {
        int files = 0;
        int dirs = 0;
        bool saw_eli98 = false;
        struct dirent *entry;
        while ((entry = readdir(listing)) != NULL)
        {
            if (entry->d_type == DT_DIR)
                dirs++;
            else
                files++;
            if (strcasecmp(entry->d_name, "eli98") == 0)
                saw_eli98 = true;
        }
        closedir(listing);
        trace("pasta D: %d arquivos, %d diretorios, eli98 %s", files, dirs,
              saw_eli98 ? "visto" : "ausente");
    }
    app().data_dir = seed;
    return true;
}

void show_progress(const std::string &text)
{
    trace("tela: %s", text.c_str());
    app().boot_error = text;
    app().screen = Screen::Boot;
    ui_draw();
}
} // namespace

bool ui_init(std::string &error)
{
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5, "1");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    // Video has to come up on its own. SDL_Init fails the whole call when any
    // flag fails, and the 01.000.005 log already had video out open before the
    // process quit. Audio and the pad driver are optional after that.
    trace("SDL_Init video, driver de video: %s",
          SDL_GetNumVideoDrivers() > 0 ? SDL_GetVideoDriver(0) : "nenhum");
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        error = SDL_GetError();
        trace("SDL_Init video falhou: %s", error.c_str());
        return false;
    }
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
        trace("audio indisponivel: %s", SDL_GetError());
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
        trace("controle indisponivel: %s", SDL_GetError());
    trace("SDL_CreateWindow");
    app().window = SDL_CreateWindow("Elifoot 98", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                    1920, 1080, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!app().window)
    {
        trace("janela 1920x1080 falhou: %s", SDL_GetError());
        app().window = SDL_CreateWindow("Elifoot 98", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
    }
    if (!app().window)
    {
        error = SDL_GetError();
        trace("janela falhou: %s", error.c_str());
        return false;
    }
    trace("criando renderer");
    const char *video_driver = SDL_GetCurrentVideoDriver();
    if (video_driver && strcmp(video_driver, "ps5") == 0)
    {
        // The PS5 backend exposes a CPU framebuffer but no window renderer.
        // Asking SDL_CreateRenderer for an accelerated renderer leaves no
        // usable fallback. Draw with SDL's software renderer directly into
        // the backend's window surface, then flip it in present().
        int window_w = 0, window_h = 0;
        SDL_GetWindowSize(app().window, &window_w, &window_h);
        trace("janela %dx%d", window_w, window_h);
        app().framebuffer = SDL_GetWindowSurface(app().window);
        if (app().framebuffer)
        {
            size_t mapped = 0;
            unsigned blocks = 0;
            bigalloc_stats(&mapped, &blocks);
            trace("framebuffer PS5 %dx%d formato 0x%x, memoria direta %zu MB em %u blocos",
                  app().framebuffer->w, app().framebuffer->h, app().framebuffer->format->format,
                  mapped >> 20, blocks);
            app().renderer = SDL_CreateSoftwareRenderer(app().framebuffer);
        }
        else
        {
            trace("SDL_GetWindowSurface falhou: %s", SDL_GetError());
        }
    }
    else
    {
        app().renderer = SDL_CreateRenderer(app().window, -1,
                                            SDL_RENDERER_ACCELERATED |
                                                SDL_RENDERER_PRESENTVSYNC);
        if (!app().renderer)
        {
            trace("renderer acelerado falhou: %s", SDL_GetError());
            app().renderer = SDL_CreateRenderer(app().window, -1, SDL_RENDERER_SOFTWARE);
        }
    }
    if (!app().renderer)
    {
        error = SDL_GetError();
        trace("renderer falhou: %s", error.c_str());
        return false;
    }
    SDL_RendererInfo info;
    int out_w = 0, out_h = 0;
    SDL_GetRendererOutputSize(app().renderer, &out_w, &out_h);
    if (SDL_GetRendererInfo(app().renderer, &info) == 0)
        trace("renderer %s, saida %dx%d", info.name, out_w, out_h);
    SDL_SetRenderDrawBlendMode(app().renderer, SDL_BLENDMODE_BLEND);
    show_progress("Iniciando...");
    for (int i = 0; i < SDL_NumJoysticks(); i++)
        input_open_pad(i);
    trace("controles: %d", SDL_NumJoysticks());
    std::string missing;
    if (!locate_data(missing))
    {
        trace("%s", missing.c_str());
        app().boot_error = missing;
        app().screen = Screen::Boot;
        return true;
    }
    install_boot_bats(app().data_dir);
    show_progress("Iniciando o DOSBox...");
    std::string boot_error;
    if (!core_boot(app().data_dir, boot_error))
    {
        trace("core_boot falhou: %s", boot_error.c_str());
        app().core_ok = false;
        app().boot_error = boot_error + "\n" + core_log();
        app().screen = Screen::Boot;
        return true;
    }
    trace("core_boot ok");
    app().core_ok = true;
    size_t bytes = 0;
    app().omatic.bind(core_ram(&bytes), bytes, path_join(app().data_dir, "cheats.txt"));
    app().screen = Screen::Playing;
    app_status("Options abre o menu. Create digita texto.");
    return true;
}

void ui_shutdown()
{
    if (app().renderer)
        SDL_DestroyRenderer(app().renderer);
    if (app().window)
        SDL_DestroyWindow(app().window);
    app().renderer = nullptr;
    app().window = nullptr;
    app().framebuffer = nullptr;
    SDL_Quit();
}

void ui_handle(const PadSnapshot &pad)
{
    if (pad.pressed[PadOptions])
    {
        if (app().screen == Screen::Playing || app().screen == Screen::Keyboard)
            open_screen(Screen::Menu);
        else
            open_screen(Screen::Playing);
        return;
    }
    if (app().screen == Screen::Playing)
    {
        if (pad.pressed[PadCreate])
            open_keyboard(EditTarget::Game);
        return;
    }
    if (app().screen == Screen::Boot)
        return;

    if (app().screen == Screen::Keyboard)
    {
        if (pad.pressed[PadUp])
            app().key_row = (app().key_row + 4) % 5;
        if (pad.pressed[PadDown])
            app().key_row = (app().key_row + 1) % 5;
        if (app().key_col >= kRowLen[app().key_row])
            app().key_col = kRowLen[app().key_row] - 1;
        if (pad.pressed[PadLeft])
            app().key_col = (app().key_col + kRowLen[app().key_row] - 1) % kRowLen[app().key_row];
        if (pad.pressed[PadRight])
            app().key_col = (app().key_col + 1) % kRowLen[app().key_row];
        if (pad.pressed[PadCircle])
            close_keyboard();
        if (pad.pressed[PadCross])
            type_key(kRows[app().key_row][app().key_col]);
        return;
    }

    if (app().screen == Screen::Text)
    {
        int scale = ui_scale();
        int rows = font_wrapped_height(1100, app().text_body, scale);
        if (pad.pressed[PadDown])
            app().text_scroll++;
        if (pad.pressed[PadUp])
            app().text_scroll--;
        if (app().text_scroll < 0)
            app().text_scroll = 0;
        if (app().text_scroll > rows)
            app().text_scroll = rows;
        if (pad.pressed[PadCircle] || pad.pressed[PadCross])
            open_screen(Screen::Menu);
        return;
    }

    if (pad.pressed[PadCircle])
    {
        if (app().screen == Screen::Menu)
            open_screen(Screen::Playing);
        else if (app().screen == Screen::Confirm)
            open_screen(Screen::Menu);
        else
            open_screen(Screen::Menu);
        return;
    }

    if (app().screen == Screen::Menu)
    {
        if (pad.pressed[PadUp])
            move_cursor(-1, 13);
        if (pad.pressed[PadDown])
            move_cursor(1, 13);
        if (pad.pressed[PadCross])
            activate_menu();
        return;
    }
    if (app().screen == Screen::Formations)
    {
        if (pad.pressed[PadUp])
            move_cursor(-1, 12);
        if (pad.pressed[PadDown])
            move_cursor(1, 12);
        if (pad.pressed[PadCross])
        {
            input_pulse((unsigned)(RETROK_F1 + app().cursor), false);
            open_screen(Screen::Playing);
        }
        return;
    }
    if (app().screen == Screen::Files)
    {
        int count = (int)app().files.size();
        if (pad.pressed[PadUp])
            move_cursor(-1, count);
        if (pad.pressed[PadDown])
            move_cursor(1, count);
        if (pad.pressed[PadCross])
            activate_file();
        return;
    }
    if (app().screen == Screen::Confirm)
    {
        if (pad.pressed[PadLeft] || pad.pressed[PadRight] || pad.pressed[PadUp] ||
            pad.pressed[PadDown])
            app().cursor = app().cursor ? 0 : 1;
        if (pad.pressed[PadCross])
        {
            if (app().cursor == 1)
                accept_confirm();
            else
                open_screen(Screen::Menu);
        }
        return;
    }
    if (app().screen == Screen::Omatic)
    {
        if (pad.pressed[PadUp])
            move_cursor(-1, 6);
        if (pad.pressed[PadDown])
            move_cursor(1, 6);
        if (pad.pressed[PadCross])
            activate_omatic();
        return;
    }
    if (app().screen == Screen::Cheats)
    {
        int count = (int)app().omatic.cheats.size();
        if (pad.pressed[PadUp])
            move_cursor(-1, count);
        if (pad.pressed[PadDown])
            move_cursor(1, count);
        if (pad.pressed[PadSquare] && count > 0)
        {
            app().omatic.remove((size_t)app().cursor);
            if (app().cursor >= count - 1)
                app().cursor = count - 2;
            if (app().cursor < 0)
                app().cursor = 0;
        }
        if (pad.pressed[PadCross] && count > 0)
        {
            app().omatic.select((size_t)app().cursor);
            open_screen(Screen::Omatic);
        }
    }
}

void ui_draw()
{
    fill(kBg);
    if (app().core_ok && (app().screen == Screen::Playing || app().screen != Screen::Boot))
        core_draw(app().renderer, app().smooth);
    if (app().screen == Screen::Playing)
    {
        unsigned long runs = 0, frames = 0;
        core_frame_info(runs, frames);
        if (frames == 0)
        {
            char line[96];
            snprintf(line, sizeof(line), "Aguardando a imagem do DOSBox (%lu ciclos)", runs);
            font_draw(app().renderer, 48, 48, line, kText, ui_scale());
        }
        draw_status(ui_scale());
        present();
        return;
    }
    if (app().screen != Screen::Boot)
        shade();
    int scale = ui_scale();
    if (app().screen == Screen::Boot)
    {
        font_draw(app().renderer, 48, 48, "Elifoot 98", kHi, scale + 1);
        font_draw_wrapped(app().renderer, 48, 120, 1200, app().boot_error, kText, scale, 0);
        present();
        return;
    }
    if (app().screen == Screen::Menu)
        draw_items("Elifoot 98", menu_items(),
                   "Cruz confirma   Círculo fecha   Options volta ao jogo");
    else if (app().screen == Screen::Formations)
    {
        std::vector<std::string> items;
        for (int i = 0; i < 12; i++)
        {
            char line[32];
            snprintf(line, sizeof(line), "F%d  %s", i + 1, kFormLabel[i]);
            items.push_back(line);
        }
        draw_items("Formações", items, "Cruz envia a tática");
    }
    else if (app().screen == Screen::Files)
    {
        std::vector<std::string> labels;
        if (app().files.empty())
            labels.push_back(app().file_kind == FileKind::Save ? "Nenhum .e98 no USB."
                                                               : "Nenhum .zip no USB.");
        for (const std::string &file : app().files)
            labels.push_back(base_name(file));
        draw_items(app().file_kind == FileKind::Save ? "Importar jogo salvo" : "Carregar patch",
                   labels, "Círculo volta");
    }
    else if (app().screen == Screen::Confirm)
    {
        font_draw(app().renderer, 48, 48, "Confirmar", kHi, scale);
        font_draw_wrapped(app().renderer, 48, 120, 1100, app().confirm_text, kText, scale, 0);
        const char *no = app().cursor == 0 ? "> Não" : "  Não";
        const char *yes = app().cursor == 1 ? "> Sim" : "  Sim";
        font_draw(app().renderer, 48, 320, no, app().cursor == 0 ? kHi : kText, scale);
        font_draw(app().renderer, 280, 320, yes, app().cursor == 1 ? kHi : kText, scale);
    }
    else if (app().screen == Screen::Text)
    {
        font_draw(app().renderer, 48, 36, app().text_title.c_str(), kHi, scale);
        font_draw_wrapped(app().renderer, 48, 100, 1200, app().text_body, kText, scale,
                          app().text_scroll);
    }
    else if (app().screen == Screen::Keyboard)
    {
        draw_keyboard();
    }
    else if (app().screen == Screen::Omatic)
    {
        font_draw_wrapped(app().renderer, 48, 36, 1200, app().omatic.info, kDim,
                          scale > 2 ? 2 : scale, 0);
        std::vector<std::string> items = omatic_items();
        int line = font_height(scale) + scale * 2;
        int y = 180;
        for (int i = 0; i < (int)items.size(); i++)
        {
            if (i == app().cursor)
            {
                SDL_SetRenderDrawColor(app().renderer, kBar.r, kBar.g, kBar.b, 255);
                SDL_Rect bar{36, y - 4, 1100, line};
                SDL_RenderFillRect(app().renderer, &bar);
            }
            font_draw(app().renderer, 48, y, items[(size_t)i].c_str(),
                      i == app().cursor ? kHi : kText, scale);
            y += line;
        }
    }
    else if (app().screen == Screen::Cheats)
    {
        std::vector<std::string> items;
        if (app().omatic.cheats.empty())
            items.push_back("Nenhum cheat salvo.");
        for (const SavedCheat &cheat : app().omatic.cheats)
        {
            char line[128];
            snprintf(line, sizeof(line), "%s  0x%X", cheat.name.c_str(), cheat.address);
            items.push_back(line);
        }
        draw_items("Cheats salvos", items, "Cruz escolhe   Quadrado apaga");
    }
    draw_status(scale);
    present();
}
