#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <iostream>
#include <string>

void toggleMute(DWORD processId) {
    //Initializes the Component Object Model (COM) library for this thread. Windows audio interface is a COM object.
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        return;
    }

    IMMDeviceEnumerator* pDEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioSessionManager2* pSManager2 = NULL;
    IAudioSessionEnumerator* pSEnumerator = NULL;

    //Creates an audio device enumerator, which is used to access audio enpoint devices.
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&pDEnumerator);

    if (FAILED(hr) || !pDEnumerator) goto cleanup;

    //Gets default playback device (eRender = output devices, eCapture = input devices)
    hr = pDEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
    if (FAILED(hr) || !pDevice) goto cleanup;

    //Activates the session manager for this device
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void**)&pSManager2);
    if (FAILED(hr) || !pSManager2) goto cleanup;

    //Gets a session enumerator to go through all active sessions
    hr = pSManager2->GetSessionEnumerator(&pSEnumerator);
    if (FAILED(hr) || !pSEnumerator) goto cleanup;

    int audioSessionCount;
    hr = pSEnumerator->GetCount(&audioSessionCount);
    if (FAILED(hr)) goto cleanup;

    //Go through all sessions and toggle mute on it if it's the correct process
    for (int i = 0; i < audioSessionCount; ++i) {
        IAudioSessionControl* control = NULL;
        IAudioSessionControl2* pSessionControl2 = NULL;
        ISimpleAudioVolume* simpleAudioVol = NULL;

        pSEnumerator->GetSession(i, &control);

        //Extended session control, needed to access process id
        hr = control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&pSessionControl2);
        if (FAILED(hr) || !pSessionControl2) {
            control->Release();
            continue;
        }

        DWORD pid;
        hr = pSessionControl2->GetProcessId(&pid);

        if (SUCCEEDED(hr) && pid == processId) {
            //Gets volume interface
            control->QueryInterface(IID_PPV_ARGS(&simpleAudioVol));

            BOOL muted;
            simpleAudioVol->GetMute(&muted);
            muted = !muted;

            simpleAudioVol->SetMute(muted, NULL);
            std::cout << "Muted: " << muted << '\n';

            simpleAudioVol->Release();
        }

        pSessionControl2->Release();
        control->Release();
    }

cleanup:
    if (pSEnumerator) pSEnumerator->Release();
    if (pSManager2) pSManager2->Release();
    if (pDevice) pDevice->Release();
    if (pDEnumerator) pDEnumerator->Release();

    CoUninitialize();
}