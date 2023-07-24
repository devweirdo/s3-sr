import os
import argparse
import struct
import math

COMMANDS_LIST = ["hello", "tell me a joke"]


def struct_pack_string(string, max_len=None):
    """
    pack string to binary data.
    if max_len is None, max_len = len(string) + 1
    else len(string) < max_len, the left will be padded by struct.pack('x')

    string: input python string
    max_len: output
    """

    if max_len == None:
        max_len = len(string)
    else:
        assert len(string) <= max_len

    left_num = max_len - len(string)
    out_bytes = None
    for char in string:
        if out_bytes == None:
            out_bytes = struct.pack("b", ord(char))
        else:
            out_bytes += struct.pack("b", ord(char))
    for i in range(left_num):
        out_bytes += struct.pack("x")
    return out_bytes


def read_data(filename):
    """
    Read binary data, like index and mndata
    """
    data = None
    with open(filename, "rb") as f:
        data = f.read()
    return data


def pack_models(model_path, out_file="srmodels.bin"):
    """
    Pack all models into one binary file by the following format:
    {
        model_num: int
        model1_info: model_info_t
        model2_info: model_info_t
        ...
        model1_index,model1_data,model1_MODEL_INFO
        model1_index,model1_data,model1_MODEL_INFO
        ...
    }model_pack_t

    {
        model_name: char[32]
        file_number: int
        file1_name: char[32]
        file1_start: int
        file1_len: int
        file2_name: char[32]
        file2_start: int   // data_len = info_start - data_start
        file2_len: int
        ...
    }model_info_t

    model_path: the path of models
    out_file: the ouput binary filename
    """

    models = {}
    file_num = 0
    model_num = 0
    for root, dirs, _ in os.walk(model_path):
        for model_name in dirs:
            models[model_name] = {}
            model_dir = os.path.join(root, model_name)
            model_num += 1
            for _, _, files in os.walk(model_dir):
                for file_name in files:
                    file_num += 1
                    file_path = os.path.join(model_dir, file_name)
                    models[model_name][file_name] = read_data(file_path)

    model_num = len(models)
    header_len = 4 + model_num * (32 + 4) + file_num * (32 + 4 + 4)
    out_bin = struct.pack("I", model_num)  # model number
    data_bin = None
    for key in models:
        model_bin = struct_pack_string(key, 32)  # + model name
        model_bin += struct.pack("I", len(models[key]))  # + file number in this model

        for file_name in models[key]:
            model_bin += struct_pack_string(file_name, 32)  # + file name
            if data_bin == None:
                model_bin += struct.pack("I", header_len)
                data_bin = models[key][file_name]
                model_bin += struct.pack("I", len(models[key][file_name]))
                # print(file_name, header_len, len(models[key][file_name]), len(data_bin))
            else:
                model_bin += struct.pack("I", header_len + len(data_bin))
                # print(file_name, header_len+len(data_bin), len(models[key][file_name]))
                data_bin += models[key][file_name]
                model_bin += struct.pack("I", len(models[key][file_name]))

        out_bin += model_bin
    assert len(out_bin) == header_len
    if data_bin != None:
        out_bin += data_bin

    out_file = os.path.join(model_path, out_file)
    with open(out_file, "wb") as f:
        f.write(out_bin)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Speech recognition commands generate tool"
    )
    parser.add_argument("-d1", "--project_path")
    parser.add_argument("-d2", "--build_path")
    args = parser.parse_args()

    target_path = args.build_path + "/srmodels"
    image_file = "srmodels.bin"
    image_file_path = target_path + "/" + image_file
    esp_sr_path = args.project_path + "/managed_components/espressif__esp-sr"
    component_hash_path = esp_sr_path + "/.component_hash"

    print("Generate speech recognition commands...")

    if os.path.exists(image_file_path):
        os.remove(image_file_path)

        f = open(target_path + "/fst/commands_en.txt", "w")
        for cmd in COMMANDS_LIST:
            f.write(str(COMMANDS_LIST.index(cmd) + 1) + " " + cmd.upper() + "\n")
        f.close()

        pack_models(target_path, image_file)
        total_size = os.path.getsize(os.path.join(target_path, image_file))
        recommended_size = int(math.ceil(total_size / 1024))
        print("Recommended model partition size: %dK" % (recommended_size))

    else:
        if os.path.exists(component_hash_path):
            os.remove(component_hash_path)

        f = open(esp_sr_path + "/model/multinet_model/fst/commands_en.txt", "w")
        for cmd in COMMANDS_LIST:
            f.write(str(COMMANDS_LIST.index(cmd) + 1) + " " + cmd.upper() + "\n")
        f.close()
