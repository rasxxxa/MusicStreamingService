#pragma once
#define _AFXDLL
#include <AfxWin.h>

using uchar = unsigned char;
using ulong = unsigned long;
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM     1
#endif

#pragma pack(1)
/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
                                    /* extra information (after cbSize) */
} MYWAVEFORMATEX;
#endif // _WAVEFORMATEX_

#pragma pack()
//----------------------------------------------------------------------------
// Class sound file access

constexpr uint32_t mmioFOURCC(const char c1, const char c2, const char c3, const char c4)
{
    return ((DWORD)(BYTE)(c1) | ((DWORD)(BYTE)(c2) << 8) |
        ((DWORD)(BYTE)(c3) << 16) | ((DWORD)(BYTE)(c4) << 24));
}



#include <filesystem>
#include <memory.h>
#include <unordered_map>
class SoundFile
{
public:
    //-- Constructor.
    SoundFile()
    {
        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));
    }

    //-- Loads the sound file (returns false when failed).
    bool LoadFile(const std::string& filePath)
    {
        ZeroMemory(&m_waveFormat, sizeof(m_waveFormat));

        CFile file;
        bool bResource = false;
        ulong ulBufferSize = 0;
        ulong ulDataSize = 0;
        if (!bResource)
        {

            CA2T f(filePath.c_str());
            if (!file.Open(f, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
            {
                return false; // not found
            }

            const ulong ulSize = 5 * sizeof(DWORD);
            uchar aucHeader[ulSize];
            if (file.Read(aucHeader, ulSize) != ulSize
                || *(ulong*)aucHeader != mmioFOURCC('R', 'I', 'F', 'F')
                || *(ulong*)(((uchar*)aucHeader) + sizeof(DWORD)) != (file.GetLength() - 2 * sizeof(DWORD))
                || *(ulong*)(((uchar*)aucHeader) + 2 * sizeof(DWORD)) != mmioFOURCC('W', 'A', 'V', 'E'))
            {
                file.Close();
                return false; // wrong format header
            }

            // check format chunk
            ulong aulChunk[2];
            aulChunk[0] = *(ulong*)(((uchar*)aucHeader) + 3 * sizeof(DWORD));
            aulChunk[1] = *(ulong*)(((uchar*)aucHeader) + 4 * sizeof(DWORD));
            const ulong ulSizePcmWaveFormat = sizeof(MYWAVEFORMATEX) - sizeof(m_waveFormat.cbSize);
            while (!(aulChunk[0] == mmioFOURCC('f', 'm', 't', ' ') && aulChunk[1] >= ulSizePcmWaveFormat))
            {
                file.Seek(aulChunk[1], CFile::current);
                if (file.Read(aulChunk, sizeof(aulChunk)) != sizeof(aulChunk))
                {
                    file.Close();
                    return false; // wrong header
                }
            }

            if (file.Read(&m_waveFormat, ulSizePcmWaveFormat) != ulSizePcmWaveFormat)
            {
                //TRACEE("ERROR: SoundFile (%s, %s): File header format (%s).\n", strPath.c_str(), strName.c_str(), strFile.c_str());
                file.Close();
                return false; // wrong header
            }

            aulChunk[1] -= ulSizePcmWaveFormat;       // ignore rest of fmt chunk

            // search data chunk
            do
            {
                file.Seek(aulChunk[1], CFile::current);
                if (file.Read(aulChunk, sizeof(aulChunk)) != sizeof(aulChunk))
                {
                    //  TRACEE("ERROR: SoundFile (%s, %s): File header data (%s).\n", strPath.c_str(), strName.c_str(), strFile.c_str());
                    file.Close();
                    return false; // wrong header
                }
            } while (aulChunk[0] != mmioFOURCC('d', 'a', 't', 'a'));

            ulDataSize = ulBufferSize = aulChunk[1];
        }

        //--------------------------------------------------------------------------
        // Sounddaten in den Speicher laden

        m_vecData.resize(ulBufferSize);
        ulong ulReadOffset = 0;
        if (file.Read(m_vecData.data() + ulReadOffset, ulDataSize) != ulDataSize)
        {
            //TRACEE("ERROR: SoundFile (%s, %s): File data (%lu).\n", strPath.c_str(), strName.c_str(), ulDataSize);
            file.Close();
            return false;                     // -> Fehler bei der Sounddatei
        }

        file.Close();
        return true;
    }

    //-- Returns wave format (DirectSound buffer needs this structure).
    MYWAVEFORMATEX GetWaveFormat() const
    {
        return m_waveFormat;
    }

    //-- Returns sound data.
    const std::vector<uchar>& GetSoundData() const
    {
        return m_vecData;
    }

private:
    MYWAVEFORMATEX m_waveFormat;
    std::vector<uchar> m_vecData;
};