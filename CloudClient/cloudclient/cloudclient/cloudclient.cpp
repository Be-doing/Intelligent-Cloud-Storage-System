#include"cloudclient.h"




bool CloudClient::Run()
{
	GetBackupInfo();
	while (1)
	{
		BackupDirListen(BACKUP_PATH);
		TouchBackupInfo();
		Sleep(10000);
	}
	return true;
}


class ThreadPut
{
private:
	std::string file_;
	int64_t range_start_;
	int64_t range_len_;
public:
	bool res_;
public:
	ThreadPut()
	{}
	ThreadPut(const std::string& filename, int64_t range_start, int64_t range_len)
		:file_(filename)
		,range_start_(range_start)
		, range_len_(range_len)
		,res_(true)
	{}
	bool Start()
	{
		std::ifstream file(file_, std::ios::binary);
		if (!file.is_open())
		{
			std::cout << "�̴߳��ļ�ʧ��" << std::endl;
			res_ = false;
			return res_;
		}
		file.seekg(range_start_, std::ios::beg);

		std::string body;
		body.resize(range_len_);
		file.read(&body[0], range_len_);
		if(!file.good())
		 {
			std::cout << "�̶߳�ȡ�ļ�ʧ��" << std::endl;
			res_ = false;
			return res_;
		}
		std::cout << "�̶߳�ȡ�ļ����" << std::endl;
		file.close();
		bf::path path(file_);
		std::string url = BACKUP_URI + path.filename().string();

		//ʵ�����ͻ���
		hb::Client client(SERVER_IP, SERVER_PORT);
		//��httpͷ��Ϣ
		hb::Headers hdr;
		hdr.insert(std::make_pair("Content-Length", std::to_string(range_len_)));

		std::stringstream tmp;
		tmp << "bytes=" << range_start_ << "-" << range_start_ + range_len_ - 1;
		hdr.insert(std::make_pair("Range", tmp.str().c_str()));
		auto rsp = client.Put(url.c_str(), hdr, body, "text/plain");
		std::cout << file_<<"=====" << range_start_ << "----" << range_len_ << "�����ϴ�" << std::endl;
		if (rsp && rsp->status == 200)
			res_ = true;
		else
			res_ = false;
		return true;
	}

};

CloudClient::CloudClient()
{
	bf::path path(BACKUP_PATH);
	if (!bf::exists(path))
		bf::create_directory(path);
}
bool CloudClient::GetBackupInfo()
{
	bf::path path_(BACKUP_INFO);
	if (!bf::exists(path_))
	{
		std::cout << "����������Ϣ�ļ�������" << std::endl;
		return false;
	}
	int64_t fsize = bf::file_size(path_);
	if (fsize == 0)
	{
		std::cout << "�����ļ���û����Ϣ��Ŀ" << std::endl;
		return false;
	}
	std::string body;
	body.resize(fsize);
	std::ifstream Infile(BACKUP_INFO, std::ios::binary);
	if (!Infile.is_open())
	{
		std::cout << "�����ļ���ʧ��" << std::endl;
		return false;
	}
	Infile.read(&body[0], fsize);
	if (!Infile.good())
	{
		std::cout << "������Ϣ��Ŀ��ȡʧ��" << std::endl;
		return false;
	}

	std::vector<std::string> list;
	boost::split(list, body, boost::is_any_of("\n"));

	for (const auto& str : list)
	{
		size_t pos = str.find(" ");
		if (pos == std::string::npos)
			continue;
		std::string key = str.substr(0, pos);
		std::string val = str.substr(pos+1);
		std::cout << key << "--->" << val << std::endl;
		backup_list_[key] = val;
	}
	std::cout << "������Ϣ��Ŀ��ȡ�ɹ�" << std::endl;
	return true;
}
bool CloudClient::TouchBackupInfo()
{
	std::cout << "����ˢ�±�����Ϣ�ļ�#################" << std::endl;
	std::string body;
	for (const auto& e : backup_list_)
	{
		body += e.first + " " + e.second + "\n";
	}
	std::ofstream Outfile(BACKUP_INFO, std::ios::binary);
	if (!Outfile.is_open())
	{
		std::cout << "�����ļ���ʧ��" << std::endl;
		return false;
	}
	Outfile.write(&body[0], body.size());
	if(!Outfile.good())
	{
		std::cout << "�����ļ�д��ʧ��" << std::endl;
		return false;
	}
	std::cout << "�����ļ��ɹ�ˢ��###################" << std::endl;
	return true;
}
bool CloudClient::BackupDirListen(const std::string& pathname)
{
	bf::directory_iterator dit_begin(pathname);
	bf::directory_iterator dit_end;
	//����Ŀ¼�Ƿ����޸Ĺ��Ļ������ļ�
	for (;dit_begin != dit_end; ++dit_begin)
	{
		//�����Ŀ¼��ݹ���м���
		if (bf::is_directory(dit_begin->status()))
		{
			BackupDirListen(dit_begin->path().string());			
			continue;
		}
		//�ж��Ƿ���Ҫ����
		if (FileDateNeedBackup(dit_begin->path().string()))
		{
			if (false == PutFileData(dit_begin->path().string()))
			{
				std::cout << dit_begin->path().string() << "����ʧ�ܣ���" << std::endl;
				continue;
			}
			std::cout << dit_begin->path().string() << "���ݳɹ�����" << std::endl;
			AddFileBackInfo(dit_begin->path().string());
		}
	}
	return true;
}
void CloudClient::AddFileBackInfo(const std::string& filename)
{
	std::cout << filename << "======" << "�������etag" << std::endl;
	std::string etag = GetFileEtag(filename);
	backup_list_[filename] = etag;
	return;
}
std::string  CloudClient::GetFileEtag(const std::string& filename)
{
	bf::path path(filename);
	//�ļ���С
	int64_t fsize = bf::file_size(path);
	//���д��ʱ��
	int64_t ltime = bf::last_write_time(path);

	std::stringstream etag;
	etag << std::hex << fsize << "-" << std::hex << ltime;
	//����etag��Ϣ
	return etag.str();
}
//������ں���
static void thread_main(ThreadPut* pthreadup)
{
	pthreadup->Start();
	return;
}
bool CloudClient::PutFileData(const std::string& filename)
{
	//���ֿ鴫���С���ļ����ݽ��зֿ鴫��
	//ѡ����̴߳���
	//1.��ȡ�ļ���С
	std::cout << "����׼���ļ��ϴ�##############" << std::endl;
	int64_t fsize = bf::file_size(filename);
	if (fsize <= 0)
	{
		std::cout << filename  <<"������" << std::endl;
		return false;
	}
	//2.����ֿ�������õ���ʼλ�ã�����С
	int count = (int)fsize / RANGE_MAX_SIZE;

	std::vector< ThreadPut> thread_put;
	std::vector< std::thread> thread_list;
	for (int i = 0; i <= count; ++i)
	{
		int64_t range_begin = i * RANGE_MAX_SIZE;
		int64_t range_len = (i + 1) * RANGE_MAX_SIZE - 1;
		if (i == count)
		{
			range_len = fsize - 1;
		}
		int64_t len = range_len - range_begin + 1;
		ThreadPut back_info(filename, range_begin, len);
		thread_put.push_back(back_info);
	}
	for (int i = 0; i <= count; ++i)
	{
		thread_list.push_back(std::thread(thread_main, &thread_put[i]));
	}
	//3.�ȴ��߳��˳����ж��ļ��ϴ����
	bool res = true;;
	for (int i = 0; i <= count; ++i)
	{
		thread_list[i].join();
		if (thread_put[i].res_ == true)
			continue;
		res = false;
	}
	//4.�ϴ��ɹ�������ļ��ı�����Ϣ
	return res;
}
bool CloudClient::FileDateNeedBackup(const std::string& filename)
{
	//��ȡ��ǰ�ļ���etag��Ϣ
	std::string etag = GetFileEtag(filename);
	//�ڳ����е�backup_list_�Ƿ��е�ǰ�ļ�����
	auto tmp = backup_list_.find(filename);
	//���Һ�backup_list_�е�etag��Ϣ��ͬ������Ҫ����
	if (tmp != backup_list_.end() && tmp->second == etag)
		return false;
	return true;
}