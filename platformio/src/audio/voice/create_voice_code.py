import os

def get_voice_header_files(folder_path):
    voice_files = []
    
    # Walk through the directory
    for root, _, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".h") and file.startswith("voice"):
                voice_files.append(file.lower())
    
    voice_files.sort()
    
    return voice_files

if __name__ == "__main__":
    folder_path = "./"  # Replace with your folder path
    voice_files = get_voice_header_files(folder_path)
    
    # print("Voice header files found:")
    for file in voice_files:
        print(f'#include "audio/voice/{file}"')
        
    print("\n\n")
    
    for file in voice_files:
        nm = file.replace(".h", "").replace("voice_","")
        sample = file.replace(".h", "")
        print(f'wav_files["{nm}"] = SFX({sample}, sizeof({sample}));')
        #wav_files["begin"] = SFX(voice_begin, sizeof(voice_begin));
        # print(file)