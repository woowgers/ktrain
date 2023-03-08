## A simple terminal keyboard trainer.
### Features:
- [x] UTF8 english and russian support
- [x] Restore screen contents after quitting
- [x] Configuration directory with train samples
- [ ] Full UTF8 support
- [ ] Adopt to changing window size
- [ ] Calculate typing speed (WPM, CPM, etc)
- [ ] Specify time of testing or number of words/character
- [ ] Custom encoding support
- [ ] Let user choose which characters will and which will not occur
- [ ] ...

### Usage

ktrain looks for first 256 files with 'txt' extension in /usr/share/ktrain directory.
By default, it chooses a random file. You can pass the basename of a file as an argument to choose one specific file.

### TODO:
* Wrap words to next screen
* Process deletions
* Fix punctuation
* Read by optimal blocks
* Refactor
* Remove messy file handling
