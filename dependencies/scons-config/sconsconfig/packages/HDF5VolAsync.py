from .Package import Package

program = r'''
int main(int argc, char* argv[]) {
  return 0;
}
'''


class HDF5VolAsync(Package):
    def __init__(self, **kwargs):
        defaults = {
            'download_url': 'https://github.com/HDFGroup/vol-async/archive/refs/tags/v1.8.1.tar.gz',
        }
        defaults.update(kwargs)
        super(HDF5VolAsync, self).__init__(**defaults)
        self.libs=[['h5async'], ['abt']]
        self.extra_libs=[
            [], ['z'], ['m'], ['z', 'm'],
        ]
        self.check_text = program

        # Setup the build handler. I'm going to assume this will work for all architectures.
        self.set_build_handler([
            "mkdir -p ${PREFIX}",
            "cd ${SOURCE_DIR} && \
            wget https://github.com/pmodels/argobots/archive/9ef9a3fc179a3b59ccdc2c2e6d6fcfbff362a329.tar.gz && \
            tar xvf 9ef9a3fc179a3b59ccdc2c2e6d6fcfbff362a329.tar.gz && \
            rm 9ef9a3fc179a3b59ccdc2c2e6d6fcfbff362a329.tar.gz && \
            rm -rf argobots && \
            mv argobots-9ef9a3fc179a3b59ccdc2c2e6d6fcfbff362a329 argobots",
            "cd ${SOURCE_DIR}/argobots && \
            ./autogen.sh && \
            ./configure --prefix=${PREFIX} && \
            make && make install",
            "cd ${SOURCE_DIR} && \
            mkdir -pv build && cd build && \
            export HDF5_DIR=${PREFIX}/../../hdf5/install && \
            export ABT_DIR=${PREFIX} && \
            cmake -DCMAKE_BUILD_TYPE=RELEASE \
                  -DCMAKE_INSTALL_PREFIX=${PREFIX} \
                  -DCMAKE_ENABLE_WRITE_MEMCPY=1 \
                  CC=mpicc \
                  .. && \
            make && make install",
        ])

    def check(self, ctx):
        env = ctx.env
        ctx.Message('Checking for HDF5VolAsync ...  ')
        self.check_options(env)

        res = super(HDF5VolAsync, self).check(ctx)

        self.check_required(res[0], ctx)
        ctx.Result(res[0])
        return res[0]
