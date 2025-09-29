""" Locus Tool Chain """

# SPDX-FileCopyrightText: 2025 Andrew C. Starritt
# SPDX-License-Identifier: GPL-3.0-only


# -----------------------------------------------------------------------------
#
class Load_Point:
    """ Singleton class to represent current address
        A bit like None
    """
    __instance = None

    def __new__(cls, *args, **kwargs):
        if cls.__instance is None:
            cls.__instance = super().__new__(cls)
        return cls.__instance

    def __repr__(self):
        return "LoadPoint"

    # Still works without overriding this, however is does save about 20 bytes.
    #
    def __reduce__(self):
        return (self.__class__, ())


LP = Load_Point()


reserved_words = ("ALP", "DATA", "BEGIN", "END", "ENTRY", "EXTERNAL", "FINISH")


# -----------------------------------------------------------------------------
#
def is_a_valid_name(name: str) -> bool:
    """ Checks if the name parameter is a valid symbil/label name.
        If not valid, the reason if provided in is_a_valid_name.message.

        Valid names start with an upper case letter, followed by upto 11
        upper-case alpha numeric characters.
        It cannot be a reserved word (including DCE reserved words).
        I think the original DataCode1 had 6 or 8  character significance.
        Extended DataCode1 had 12, and that is what I am going with here.
    """

    is_a_valid_name.message = None

    if not isinstance(name, str):
        is_a_valid_name.message = "the name not a string"
        return False

    if name in reserved_words:
        is_a_valid_name.message = f"'{name}' is a reserved word"
        return False

    n = len(name)
    if n < 1 or n > 12:
        is_a_valid_name.message = f"'{name}' length {n} not in range 1 to 12"
        return False

    if not ('A' <= name[0] <= 'Z'):
        is_a_valid_name.message = f"'{name}' does not start with an upper case letter"
        return False

    for x in name[1:]:
        if not ('A' <= x <= 'Z') and not ('0' <= x <= '9'):
            is_a_valid_name.message = f"'{x}' in '{name}' is not an upper case alpha numeric character"
            return False

    is_a_valid_name.message = "ok"
    return True


# end
