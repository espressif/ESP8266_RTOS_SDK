import os


class Cert(object):
    def __init__(self, name, buff):
        self.name = name
        self.len = len(buff)
        self.buff = buff
        pass
    
    def __str__(self):
        out_str = ['\0']*32
        for i in range(len(self.name)):
            out_str[i] = self.name[i]
        out_str = "".join(out_str)
        out_str += str(chr(self.len & 0xFF))
        out_str += str(chr((self.len & 0xFF00) >> 8))
        out_str += self.buff
        return out_str
        pass


def main():
    cert_list = []
    file_list = os.listdir(os.getcwd())
    cert_file_list = []
    for _file in file_list:
        pos = _file.find(".key_1024")
        if pos != -1:
            cert_file_list.append(_file[:pos])
        
        pos = _file.find(".cer")
        if pos!= -1:
            cert_file_list.append(_file[:pos])

    for cert_file in cert_file_list:
        if cert_file == 'private_key':
            with open(cert_file+".key_1024", 'rb') as f:
                buff = f.read()
            cert_list.append(Cert(cert_file, buff))

        if cert_file == 'certificate':    
            with open(cert_file+".cer", 'rb') as f:
                buff = f.read()
            cert_list.append(Cert(cert_file, buff))
        
    with open('esp_cert_private_key.bin', 'wb+') as f:
        for _cert in cert_list:
            f.write("%s" % _cert)
    pass
if __name__ == '__main__':
    main()

