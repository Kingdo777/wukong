from setuptools import setup

setup(
    name='wk',
    version='0.1.0',
    packages=['wk', 'wk.utils'],
    install_requires=['invoke'],
    entry_points={
        'console_scripts': ['wk = wk.main:program.run']
    }
)
