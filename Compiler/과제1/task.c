#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct numtype // 수식의 값이 정수인지 실수인지 구분하는데 쓰이는 자료형
{
    enum{INT, REAL} t;
    union value
    {
        int ival;
        float fval;
    } val;
} NType;

NType num; // 토큰이 NUMBER일 경우, 그 값을 저장하는 변수
enum{EXTRA, NUMBER, PLUS, STAR, LP, RP, END, SLASH, PER} token = NUMBER, pretoken; // token에는 현재 읽어들인 토큰의 종류가 저장된다. pretoken은 term에서 연산식을 저장한다.

int flag = 0; // '-'기호가 나왔다면 1, 아니면 0
char ch = ' '; // getchar로 읽어들인 문자를 저장한다.

NType expression();
NType term();
NType factor();
void get_token();
void error(int i);
NType add(NType a, NType b); // 두 값의 합을 구한다.
NType mul(NType a, NType b); // 두 값의 곱을 구한다.
NType share(NType a, NType b); // 앞의 값을 뒤의 값으로 나눈 몫을 구한다.
NType remain(NType a, NType b); // 앞의 값을 뒤의 값으로 나눈 나머지를 구한다.

void main()
{
    NType result;
    get_token();
    result = expression();
    if(token != END)
        error(3);
    else
    {
        if(result.t == 1)
            printf("%f\n", result.val.fval);
        else
            printf("%d\n", result.val.ival);
    }
}

NType expression()
{
    NType result, tval;
    if(token == PLUS)
        get_token();
    result = term();
    while(token == PLUS)
    {
        get_token();
        tval = term();
        result = add(result, tval);
    }
    return result;
}

NType term()
{
    NType result, tval;
    int tflag = 0; // factor()의 결과값의 부호를 정해서 저장한다.
    if(flag == 1)
    {
        tflag = 1;
        flag = 0;
    }
    result = factor();
    if(tflag == 1) // tflag 값에 따라 부호를 바꾼다.
    {
        if(result.t == REAL)
            result.val.fval *= -1;
        else
            result.val.ival *= -1;
        tflag = 0;
    }
    while(token == STAR || token == SLASH || token == PER)
    {
        pretoken = token; // 다음 factor()의 결과값으로 수행할 연산을 정해서 저장한다.
        get_token();
        tval = factor();
        if(pretoken == STAR)
            result = mul(result, tval);
        else if(pretoken == SLASH)
            result = share(result, tval);
        else if(pretoken == PER)
            result = remain(result, tval);
    }
    return result;
}

NType factor()
{
    NType result;
    if(token == NUMBER)
    {
        result = num;
        get_token();
    }
    else if(token == LP)
    {
        get_token();
        result = expression();
        if(token == RP)
            get_token();
        else
            error(2);
    }
    else
        error(1);
    return result;
}

void get_token()
{
    while(ch == ' ')
    {
        ch = getchar();
    }

    if('0' <= ch && ch <= '9')
    {
        int result = 0;
        token = NUMBER;
        do
        {
           result = result * 10 + ch - '0';
           ch = getchar();
        } while ('0' <= ch && ch <= '9');
        
        if(ch == '.')
        {
            ch = getchar();
            if('0' <= ch && ch <= '9')
            {
                float fpart = 0.0; // 소숫점 아래 자리 부분을 저장한다.
                int n = 0; // 소숫점 자릿수
                do
                {
                    n++;
                    fpart = fpart * 10 + ch - '0';
                    ch = getchar();
                } while ('0' <= ch && ch <= '9');
                
                while(n--)
                {
                    fpart *= 0.1;
                }

                num.t = REAL;
                num.val.fval = (float)result + fpart;
            }
            else
            {
                error(4);
            }
        }
        else
        {
            num.t = INT;
            num.val.ival = result;
        }
    }
    else
    {
        switch(ch)
        {
            case '-': flag = 1;
            case '+': token = PLUS; break;
            case '*': token = STAR; break;
            case '(': token = LP; break;
            case ')': token = RP; break;
            case '/': token = SLASH; break;
            case '%': token = PER; break;
            case '\n': 
            case EOF : token = END; break;
            default: token = EXTRA; break;
        }
        ch = ' ';
    }
}

void error(int i)
{
    switch(i)
    {
        case 1: printf("NUMBER or Left Parenthesis expected\n"); break;
        case 2: printf("Right Parenthesis expected\n"); break;
        case 3: printf("EOF expected\n"); break;
        case 4: printf("Integer expected after '.'\n"); break;
        case 5: printf("ZERO is not expected\n"); break;
        case 6: printf("NUMBER or Parenthesis expected before '-'\n"); break;
    }
    exit(1);
}

NType add(NType a, NType b)
{
    NType result;

    if(a.t == INT && b.t == INT)
    {
        result.t = INT;
        result.val.ival = a.val.ival + b.val.ival;
    }
    else
    {
        result.t = REAL;
        float tval = 0;

        if(a.t == INT)
        {
            tval += (float)a.val.ival;
        }
        else
        {
            tval += a.val.fval;
        }
        
        if(b.t == INT)
        {
            tval += (float)b.val.ival;
        }
        else
        {
            tval += b.val.fval;
        }

        result.val.fval = tval;
    }

    return result;
}

NType mul(NType a, NType b)
{
    NType result;

    if(a.t == INT && b.t == INT)
    {
        result.t = INT;
        result.val.ival = a.val.ival * b.val.ival;
    }
    else
    {
        result.t = REAL;
        float tval = 1;

        if(a.t == INT)
        {
            tval *= (float)a.val.ival;
        }
        else
        {
            tval *= a.val.fval;
        }
        
        if(b.t == INT)
        {
            tval *= (float)b.val.ival;
        }
        else
        {
            tval *= b.val.fval;
        }

        result.val.fval = tval;
    }

    return result;
}

NType share(NType a, NType b)
{
    NType result;

    if(a.t == INT && b.t == INT)
    {
        result.t = INT;
        if(b.val.ival == 0)
            error(5);
        result.val.ival = a.val.ival / b.val.ival;
    }
    else
    {
        result.t = REAL;
        float tval;

        if(a.t == INT)
        {
            tval = (float)a.val.ival;
        }
        else
        {
            tval = a.val.fval;
        }
        
        if(b.t == INT)
        {
            if(b.val.ival == 0)
                error(5);
            tval /= (float)b.val.ival;
        }
        else
        {
            if(b.val.fval == 0)
                error(5);
            tval /= b.val.fval;
        }

        result.val.fval = tval;
    }

    return result;
}

NType remain(NType a, NType b)
{
    NType result;

    if(a.t == INT && b.t == INT)
    {
        result.t = INT;
        if(b.val.ival == 0)
            error(5);
        result.val.ival = a.val.ival % b.val.ival;
    }
    else
    {
        result.t = REAL;
        float tval;

        if(a.t == INT)
        {
            tval = (float)a.val.ival;
        }
        else
        {
            tval = a.val.fval;
        }
        
        if(b.t == INT)
        {
            if(b.val.ival == 0)
                error(5);
            tval = fmodf(tval, (float)b.val.ival);
        }
        else
        {
            if(b.val.fval == 0)
                error(5);
            tval = fmodf(tval, b.val.fval);
        }

        result.val.fval = tval;
    }

    return result;
}