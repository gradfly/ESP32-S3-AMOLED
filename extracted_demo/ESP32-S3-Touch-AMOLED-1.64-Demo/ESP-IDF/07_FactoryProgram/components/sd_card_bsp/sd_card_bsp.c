#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "sd_card_bsp.h"

#define PIN_NUM_MISO  40
#define PIN_NUM_MOSI  39
#define PIN_NUM_CLK   41
#define PIN_NUM_CS    38
#define EXAMPLE_MAX_CHAR_SIZE    64  //读取数据的最大值
#define SDlist "/sd_card" //目录,类似于一个标准



sdmmc_card_t *card = NULL;

void SD_card_Init(void)
{
  esp_vfs_fat_sdmmc_mount_config_t mount_config = 
  {
    .format_if_mount_failed = true,    //如果挂靠失败，创建分区表并格式化SD卡
    .max_files = 5,                   //打开文件最大数
    .allocation_unit_size = 1024 * 1024 //类似扇区大小
  };

  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = SDMMC_FREQ_HIGHSPEED; //高速

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;                    //1线
  slot_config.clk = PIN_NUM_CLK;
  slot_config.cmd = PIN_NUM_MOSI;
  slot_config.d0 = PIN_NUM_MISO;
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_vfs_fat_sdmmc_mount(SDlist, &host, &slot_config, &mount_config, &card));

  if(card != NULL)
  {
    sdmmc_card_print_info(stdout, card); //把卡的信息打印出来
    //printf("sios:%.2f\n",(float)(card->csd.capacity)/2048/1024);//g
  }
  //char list[30];
  //sprintf(list,"%s%s",SDlist,"Waveshare Image1.png");
  //s_example_read_file(list);
}
float sd_cadr_get_value(void)
{
  if(card != NULL)
  {
    return (float)(card->csd.capacity)/2048/1024; //G
  }
  else
  return 0;
}

/*写数据
path:路径
data:数据
*/
esp_err_t s_example_write_file(const char *path, char *data)
{
  esp_err_t err;
  if(card == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); //先检查是否有SD卡
  if(err != ESP_OK)
  {
    return err;
  }
  FILE *f = fopen(path, "w"); //获取路径地址
  if(f == NULL)
  {
    printf("path:Write Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fprintf(f, data); //写入
  fclose(f);
  return ESP_OK;
}
/*
读数据
path:路径
*/
esp_err_t s_example_read_file(const char *path,uint8_t *pxbuf,uint32_t *outLen)
{
  esp_err_t err;
  if(card == NULL)
  {
    printf("path:card == NULL\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); //先检查是否有SD卡
  if(err != ESP_OK)
  {
    printf("path:card == NO\n");
    return err;
  }
  FILE *f = fopen(path, "rb");
  if (f == NULL)
  {
    printf("path:Read Wrong path\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END);     //把指针移到最后面
  uint32_t unlen = ftell(f);
  //fgets(pxbuf, unlen, f); //读取 文本
  fseek(f, 0, SEEK_SET); //把指针移到最前面
  uint32_t poutLen = fread((void *)pxbuf,1,unlen,f);
  printf("pxlen: %ld,outLen: %ld\n",unlen,poutLen);
  *outLen = poutLen;
  fclose(f);
  return ESP_OK;
}
/*
*s_example_jpeg_file：读取jpeg图片的数据函数
path：需要打开的文件路径
intbuf：读取到的数据缓冲区
outlen：缓冲区的字节数
*/
esp_err_t s_example_jpeg_file(const char *path,uint8_t *outbuf,uint32_t *outLen)
{
  esp_err_t err;
  if(card == NULL)
  {
    printf("ok1\n");
    return ESP_ERR_NOT_FOUND;
  }
  err = sdmmc_get_status(card); //先检查是否有SD卡
  if(err != ESP_OK)
  {
    printf("ok2\n");
    return err;
  }
  FILE *f = fopen(path, "rb"); //使用二进制读取数据
  if(f == NULL)
  {
    printf("ok3\n");
    return ESP_ERR_NOT_FOUND;
  }
  fseek(f, 0, SEEK_END);      //把指针移到最后面
  uint32_t unlen = ftell(f);  //返回指针当前的位置
  fseek(f, 0, SEEK_SET);      //把指针移到最前面
  uint32_t poutLen = fread((void *)outbuf,1,unlen,f); //读取二进制数据
  if(poutLen != unlen)
  return ESP_ERR_NOT_FOUND;
  *outLen = poutLen;
  fclose(f);                //关闭文件
  return ESP_OK;
}
/*
struct stat st;
stat(file_foo, &st);//获取文件信息 成功返回0  file_foo是字符串，文件名字需要后缀 可以表示是不是有该文件
unlink(file_foo);//删除文件 成功返回0
rename(char*,char*);//重命名文件
esp_vfs_fat_sdcard_format();//格式化
esp_vfs_fat_sdcard_unmount(mount_point, card);//卸载vfs
FILE *fopen(const char *filename, const char *mode);
"w": 以写入模式打开文件，如果文件存在，则清空文件内容；如果文件不存在，则创建新文件。
"a": 以追加模式打开文件，如果文件不存在，则创建新文件。
mkdir(dirname, mode);创建文件夹

读取其他不是文本类型数据"rb" 模式用于以只读和二进制的方式打开文件，适用于图像等二进制文件。
如果你只使用 "r"，则会以文本模式打开文件，这可能导致读取二进制文件时出现数据损坏或错误。
因此，对于图像文件（如 JPEG、PNG 等），你应该使用 "rb" 模式来确保正确读取文件内容。
b转换成二进制
通过下面两个函数可以进行文件大小返回
  fseek(file, 0, SEEK_END)：将文件指针移动到文件的末尾。
  ftell(file)；返回当前文件的指针位置，就是文件大小，按字节算
*/