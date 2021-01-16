# pavolume

pavolume is a volume control and monitoring utility for
[PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/).

pavolume is used by [BBS - BSPWM Bar Scripts](https://github.com/andornaut/bbs)
to render a bar (top panel) volume widget.

![Screenshot](https://raw.githubusercontent.com/andornaut/pavolume/master/screenshot.png)

## Usage

```bash
make
sudo make install
pavolume
```

## Options

Flag|Description
---|---
-h|Print help text
-s|Monitoring mode: Print volume level whenever it changes
-f format|[printf](https://en.wikipedia.org/wiki/Printf_format_string) format string. Default: `"%s"`
-m on\|off\|toggle|Muting options
-v [-\|+]number|Volume level. A number between 0 and 100 inclusive. Optionally prefixed with `-` or `+` to denote a delta.

### Examples

```bash
$ pavolume -h
pavolume [-h|-s|-f format|-m [on|off|toggle]|-v [+|-]number]

$ Print volume levels whenever they change (prefix with "V"). Set initial volume to 50%. Implicitly disable muting.
pavolume -sf "S%s" -v 50
```

## Runtime Dependencies

* [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)

## Build Dependencies

* libpulse (eg. [libpulse-dev](https://packages.ubuntu.com/search?keywords=libpulse-dev))
