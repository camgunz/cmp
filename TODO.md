# To Do

- Work on fixing double-copy issue
  - Essentially everything is written to a `cmp_object_t` before it's written
    out to the caller, which is inefficient.  The reasoning for this is to not
    pollute the caller's environment should an error occur, but in practice
    it's probably better to say, "you can't trust the contents of output
    arguments you pass to CMP if the call fails".

- Build real docs
  - Probably still just a Markdown file, but still, things have gotten complex
    enough that `cmp.h` and `README.md` don't really cover it anymore.

- Prevent users from using extended types < 0 (reserved by MessagePack)
