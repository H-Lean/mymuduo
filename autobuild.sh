set -e

# 如果没有build目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build/ && 
    cmake .. &&
    make
# 返回根目录
cd ..

# 头文件和so库都复制到系统路径
# 头文件拷贝到 /usr/include/mymuduo
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi
for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done
# so库拷贝到 /usr/lib（环境变量路径）
cp `pwd`/lib/libmymuduo.so /usr/lib
# 刷新
ldconfig