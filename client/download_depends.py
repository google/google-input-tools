import os
import urllib2
import tarfile
import zipfile

def download_and_extract(url, dest):
  f = urllib2.urlopen(url)
  if url.endswith('.zip'):
    file = 'download.zip'
  elif url.endswith('.tar.gz'):
    file = 'download.tar.gz'

  with open(file, "wb") as code:
     code.write(f.read())
  if zipfile.is_zipfile(file):
    zipfile.ZipFile(file).extractall(dest)
  elif tarfile.is_tarfile(file):
  	tarfile.open(file, 'r:gz').extractall(dest)
  os.remove(file)

FILE_LIST = [
  {
    'url' : 'https://protobuf.googlecode.com/files/protoc-2.5.0-win32.zip',
    'dest' : 'depends/',
  },
  {
    'url' : 'https://protobuf.googlecode.com/files/protobuf-2.5.0.zip',
    'dest' : 'depends/',
  },
  {
    'url' : 'https://googletest.googlecode.com/files/gtest-1.7.0.zip',
    'dest' : 'depends/',
  },
  {
    'url' : 'http://zlib.net/zlib-1.2.8.tar.gz',
    'dest' : 'depends/',
  },
  {
    'url' : 'http://jaist.dl.sourceforge.net/project/wtl/WTL%208.0/WTL%208.0%20Final/WTL80_7161_Final.zip',
    'dest' : 'depends/wtl80/',
  }

]

if __name__ == '__main__':
  if not os.path.exists('depends'):
    os.mkdir('depends')
  for entry in FILE_LIST:
    download_and_extract(entry['url'], entry['dest'])
    print entry['url']
  