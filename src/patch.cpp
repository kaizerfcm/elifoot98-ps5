/* Elifoot 98 PS5 title. Copyright (C) 2026 kaizerfcm. SPDX-License-Identifier: GPL-2.0-or-later */
#include "patch.h"

#include "util.h"
#include "zip.h"

#include <string.h>

namespace
{
const char *kUsb[] = {"/mnt/usb0", "/mnt/usb1", "/usb0", "/usb1", "/data"};

const char kElifootBat[] = "D:\r\n"
                           "cd ELI98\r\n"
                           "CALL D:\\PATCH.BAT\r\n"
                           "\r\n"
                           "win D:\\ELI98\\ELIFOOT.EXE\r\n";

const char kPatchBat[] = "cd D:\\ELI98\\PATCH\r\n"
                         "\r\n"
                         "IF NOT EXIST FLAGS GOTO EQUIPAS\r\n"
                         "DEL ..\\FLAGS\\*.*\r\n"
                         "COPY FLAGS\\*.* ..\\FLAGS\r\n"
                         "ECHO \"PATCHED FLAGS\"\r\n"
                         "\r\n"
                         ":EQUIPAS\r\n"
                         "IF NOT EXIST EQUIPAS GOTO COUNTRY\r\n"
                         "DEL ..\\EQUIPAS\\*.*\r\n"
                         "COPY EQUIPAS\\*.* ..\\EQUIPAS\r\n"
                         "ECHO \"PATCHED EQUIPAS\"\r\n"
                         "\r\n"
                         ":COUNTRY\r\n"
                         "IF NOT EXIST COUNTRY.TXE GOTO PLOP\r\n"
                         "DEL ..\\COUNTRY.TXE\r\n"
                         "COPY COUNTRY.TXE ..\\COUNTRY.TXE\r\n"
                         "ECHO \"PATCHED COUNTRY.TXE\"\r\n"
                         "\r\n"
                         ":PLOP\r\n"
                         "IF NOT EXIST CTRGROUP GOTO PATCHEND\r\n"
                         "\r\n"
                         "cd CTRGROUP\r\n"
                         "IF NOT EXIST PLOP.TXE GOTO BOSMAN\r\n"
                         "DEL ..\\..\\CTRGROUP\\PLOP.TXE\r\n"
                         "COPY PLOP.TXE ..\\..\\CTRGROUP\\PLOP.TXE\r\n"
                         "ECHO \"PATCHED CTRGROUP\\PLOP.TXE\"\r\n"
                         "\r\n"
                         ":BOSMAN\r\n"
                         "IF NOT EXIST BOSMAN.TXE GOTO PATCHEND\r\n"
                         "DEL ..\\..\\CTRGROUP\\BOSMAN.TXE\r\n"
                         "COPY BOSMAN.TXE ..\\..\\CTRGROUP\\BOSMAN.TXE\r\n"
                         "ECHO \"PATCHED CTRGROUP\\BOSMAN.TXE\"\r\n"
                         "\r\n"
                         ":PATCHEND\r\n"
                         "cd D:\\ELI98\r\n"
                         "\r\n"
                         "ECHO \"PATCH_END\"\r\n";

bool contains_ci(const std::string &hay, const char *needle)
{
    return lower_ascii(hay).find(needle) != std::string::npos;
}
} // namespace

void install_boot_bats(const std::string &data_dir)
{
    write_text(path_join(data_dir, "d/ELIFOOT.BAT"), kElifootBat);
    write_text(path_join(data_dir, "d/PATCH.BAT"), kPatchBat);
}

std::vector<std::string> find_patch_zips()
{
    std::vector<std::string> files;
    for (const char *root : kUsb)
    {
        if (!path_is_dir(root))
            continue;
        list_files(root, ".zip", true, files);
    }
    return files;
}

std::string prepare_patch(const std::string &zip_path, std::vector<PatchFile> &files)
{
    files.clear();
    std::vector<ZipEntry> entries;
    std::string error;
    if (!zip_read(zip_path, entries, error))
        return error;
    int equipas = 0;
    for (ZipEntry &entry : entries)
    {
        std::string low = lower_ascii(entry.name);
        std::string file = upper_ascii(base_name(entry.name));
        PatchFile out;
        out.data = std::move(entry.data);
        if (low.size() >= 4 && low.compare(low.size() - 4, 4, ".eft") == 0)
        {
            out.relative = "EQUIPAS/" + file;
            equipas++;
        }
        else if (contains_ci(low, "country.txe"))
        {
            out.relative = "COUNTRY.TXE";
        }
        else if (contains_ci(low, "referee.txe"))
        {
            out.relative = "REFEREE.TXE";
        }
        else if (contains_ci(low, "ctrgroup") && low.size() >= 4 &&
                 low.compare(low.size() - 4, 4, ".txe") == 0)
        {
            out.relative = "CTRGROUP/" + file;
        }
        else if (contains_ci(low, "flags") && low.size() >= 4 &&
                 low.compare(low.size() - 4, 4, ".bmp") == 0)
        {
            out.relative = "FLAGS/" + file;
        }
        else
        {
            continue;
        }
        files.push_back(std::move(out));
    }
    if (equipas > 0 && equipas < 34)
    {
        files.clear();
        return "Nao ha equipes suficientes para jogar. O patch deve conter pelo menos 34 arquivos "
               ".EFT.";
    }
    if (files.empty())
        return "O zip nao contem equipes, bandeiras ou arquivos de configuracao.";
    return "";
}

std::string apply_patch(const std::string &data_dir, const std::vector<PatchFile> &files)
{
    std::string root = path_join(data_dir, "d/eli98/PATCH");
    if (!mkdir_p(root))
        return "Nao foi possivel criar a pasta PATCH.";
    for (const PatchFile &file : files)
    {
        std::string path = path_join(root, file.relative);
        if (!write_file(path, file.data.data(), file.data.size()))
            return "Nao foi possivel gravar um arquivo do patch.";
    }
    install_boot_bats(data_dir);
    return "";
}

std::string clear_patch(const std::string &data_dir)
{
    remove_tree(path_join(data_dir, "d/eli98/PATCH"));
    install_boot_bats(data_dir);
    return "Patch removido. Times e bandeiras voltam ao original no proximo boot.";
}
