#### what is this ?
quick and dirty blogging SofTwArE

#### rationale ?
write and manage content in markdown with minimal effort/footprint/cpu-cycles/interaction wizza cumba-yotar

#### how to build ?
- do a recursive clone
```
git clone --recursive --shallow-submodules git@github.com:demon36/plebspot.git
```
- cd and make the program
```
make
```
- psst: build config is not so windows friendly but you can work your way through using mingw + gcc/clang

#### how to use ?
- run plebspot
- place markdown files in `pages` and `posts` directories

#### roadmap
- [ ] use config file for icon, title, port, etc
- [ ] code blocks syntax highlighting (implementation options: enscript, gnu src-highlitem, highlightjs)
- [ ] optional custom post/page title instead of filename
- [ ] basic html templates
- [ ] post categories
- [ ] pages hierarchy
- [ ] compressed html cache
- [ ] rss feeds
- [ ] comments
