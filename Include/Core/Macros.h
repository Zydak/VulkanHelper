#pragma once

#define DEFINE_BITWISE_OPERATORS(ENUM_CLASS)\
inline ENUM_CLASS operator|(ENUM_CLASS left, ENUM_CLASS right)\
{\
    return ENUM_CLASS((int)left | int(right));\
}\
\
inline ENUM_CLASS operator&(ENUM_CLASS left, ENUM_CLASS right)\
{\
    return ENUM_CLASS((int)left | int(right));\
}\
\
inline ENUM_CLASS operator~(ENUM_CLASS val)\
{\
    return ENUM_CLASS(~(int)val);\
}\
\
inline ENUM_CLASS operator^(ENUM_CLASS left, ENUM_CLASS right)\
{\
    return ENUM_CLASS((int)left ^ int(right));\
}\
\
inline ENUM_CLASS& operator|=(ENUM_CLASS& left, ENUM_CLASS right)\
{\
    left = left | right;\
    return left;\
}\
\
inline ENUM_CLASS& operator&=(ENUM_CLASS& left, ENUM_CLASS right)\
{\
    left = left & right;\
    return left;\
}\
\
inline ENUM_CLASS& operator^=(ENUM_CLASS& left, ENUM_CLASS right)\
{\
    left = left ^ right;\
    return left;\
}
