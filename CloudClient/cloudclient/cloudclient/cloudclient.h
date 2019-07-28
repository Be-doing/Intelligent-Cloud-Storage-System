#pragma once

#include<iostream>
#include<string>
#include<vector>
#include<sstream>
#include<unordered_map>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>//�ַ����и�
#include<thread>
#include"httplib.h"

#define BACKUP_PATH "back"		//�ļ�����뱸��Ŀ¼
#define BACKUP_INFO "backupinfo.txt"		//�ļ�������Ϣ
#define RANGE_MAX_SIZE (10<<20)
#define SERVER_IP "192.168.138.135"
#define SERVER_PORT 9090
#define BACKUP_URI "/list/"
namespace bf = boost::filesystem;
namespace hb = httplib;
//����
//��ȡ�ļ�������Ϣ
//���Ŀ¼���ļ���Ϣ
//�ж��ļ��Ƿ���Ҫ����
		//mtime �ļ��޸�ʱ�� �� fsize �ļ���С �Ƿ����ϴ���ͬ	
//�����ļ�
//��¼�ļ�������Ϣ����etag = mtime + fsize
		//mtime���ļ��޸�ʱ��
		//fsize���ļ���С

class CloudClient
{
public:
	CloudClient();
	bool Run();
private:
	//��ȡ������Ϣ
	bool GetBackupInfo();
	//ˢ�±�����Ϣ
	bool TouchBackupInfo();
	//��������Ŀ¼
	bool BackupDirListen(const std::string& pathname);
	//�ϴ��ļ�/�ļ�����
	bool PutFileData(const std::string& filename);
		//��ӵ����ļ���etag
		void AddFileBackInfo(const std::string& filename);
		//��ȡ�����ļ�etag
		std::string GetFileEtag(const std::string& filename);
	//�ļ��Ƿ���Ҫ����
	bool FileDateNeedBackup(const std::string& filename);
private:
	//������Ϣ
	std::unordered_map<std::string, std::string> backup_list_;
};