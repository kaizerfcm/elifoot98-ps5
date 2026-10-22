/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "saves.h"

#include "util.h"

#include <stdio.h>
#include <unistd.h>

namespace
{
const char *kUsb[] = {"/mnt/usb0", "/mnt/usb1", "/usb0", "/usb1", "/data"};

std::string export_dir()
{
    for (const char *root : kUsb)
    {
        if (path_is_dir(root))
            return path_join(root, "elifoot98-saves");
    }
    return "/data/elifoot98-export";
}
} // namespace

std::string saves_folder(const std::string &data_dir)
{
    return path_join(data_dir, "d/eli98/jogos");
}

std::string export_saves(const std::string &data_dir)
{
    std::vector<std::string> files;
    list_files(saves_folder(data_dir), ".e98", false, files);
    if (files.empty())
        return "Nao ha jogos salvos para exportar.";
    std::string dest = export_dir();
    if (!mkdir_p(dest))
        return "Nao foi possivel criar a pasta de exportacao.";
    int copied = 0;
    for (const std::string &file : files)
    {
        if (copy_file(file, path_join(dest, upper_ascii(base_name(file)))))
            copied++;
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "%d jogo(s) exportado(s) para %s", copied, dest.c_str());
    return buf;
}

std::vector<std::string> find_import_saves()
{
    std::vector<std::string> files;
    for (const char *root : kUsb)
    {
        if (!path_is_dir(root))
            continue;
        list_files(root, ".e98", true, files);
        list_files(path_join(root, "elifoot98-saves"), ".e98", false, files);
    }
    list_files("/data/elifoot98-export", ".e98", false, files);
    return files;
}

std::string import_save(const std::string &data_dir, const std::string &file)
{
    std::string name = upper_ascii(base_name(file));
    if (lower_ascii(name).size() < 4 ||
        lower_ascii(name).compare(lower_ascii(name).size() - 4, 4, ".e98") != 0)
    {
        return "Somente arquivos .e98 podem ser importados.";
    }
    std::string dest = path_join(saves_folder(data_dir), name);
    if (!mkdir_p(saves_folder(data_dir)))
        return "Nao foi possivel abrir a pasta de jogos.";
    if (!copy_file(file, dest))
        return "Nao foi possivel copiar o jogo salvo.";
    return "Jogo salvo importado. O jogo sera reiniciado.";
}

std::string wipe_data(const std::string &data_dir)
{
    std::string jogos = saves_folder(data_dir);
    std::vector<std::string> files;
    list_files(jogos, ".e98", false, files);
    for (const std::string &file : files)
        unlink(file.c_str());
    remove_tree(path_join(data_dir, "d/eli98/PATCH"));
    const char *seed = "/app0/assets/elifoot98/hdd.img";
    std::string work = path_join(data_dir, "hdd.img");
    if (!path_is_file(seed) || !copy_file(seed, work))
        return "Nao foi possivel restaurar o disco do Windows.";
    return "Dados apagados. O jogo sera reiniciado.";
}
