#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

double parseNumber(const char **expr)
{
    double result = 0;
    double decimal = 0;
    int decimalPlace = 0;

    while (isdigit(**expr))
    {
        result = result * 10 + (**expr - '0');
        (*expr)++;
    }

    if (**expr == '.')
    {
        (*expr)++;
        while (isdigit(**expr))
        {
            decimal = decimal * 10 + (**expr - '0');
            (*expr)++;
            decimalPlace++;
        }

        while (decimalPlace > 0)
        {
            decimal /= 10;
            decimalPlace--;
        }
        result += decimal;
    }

    return result;
}

void skipWhitespace(const char **expr)
{
    while (isspace(**expr))
    {
        (*expr)++;
    }
}

double parseExpression(const char **expr);

double parseFactor(const char **expr)
{
    skipWhitespace(expr);
    double result = 0;

    if (**expr == '(')
    {
        (*expr)++;
        result = parseExpression(expr);
        skipWhitespace(expr);
        if (**expr == ')')
        {
            (*expr)++;
        }
    }
    else if (**expr == '-')
    {
        (*expr)++;
        result = -parseFactor(expr);
    }
    else if (**expr == '+')
    {
        (*expr)++;
        result = parseFactor(expr);
    }
    else
    {
        result = parseNumber(expr);
    }

    skipWhitespace(expr);
    return result;
}

double parseTerm(const char **expr)
{
    double result = parseFactor(expr);

    while (**expr == '*' || **expr == '/')
    {
        char operator = **expr;
        (*expr)++;
        double operand = parseFactor(expr);

        if (operator == '*')
        {
            result *= operand;
        }
        else
        {
            if (operand == 0)
            {
                printf("Error: Division by zero\n");
                exit(0);
            }
            result /= operand;
        }
    }

    return result;
}

double parseExpression(const char **expr)
{
    double result = parseTerm(expr);

    while (**expr == '+' || **expr == '-')
    {
        char operator = **expr;
        (*expr)++;
        double operand = parseTerm(expr);

        if (operator == '+')
        {
            result += operand;
        }
        else
        {
            result -= operand;
        }
    }

    return result;
}

double evaluateExpression(const char *expression)
{
    const char *expr = expression;
    skipWhitespace(&expr);

    if (*expr == '\0')
    {
        printf("Error: Invalid expression\n");
        exit(0);
    }

    double result = parseExpression(&expr);
    skipWhitespace(&expr);
    if (*expr != '\0')
    {
        printf("Error: not complete expression, remain %s", expr);
        exit(0);
    }

    return result;
}

void testExpression(const char *expression)
{
    printf("Expression: %s\n", expression);
    double result = evaluateExpression(expression);
    printf("Result: %.2f\n", result);
}

int main()
{
    printf("=== 字符串表达式解析器 ===\n\n");

    // 测试各种表达式
    testExpression("123+456");
    testExpression("100-50");
    testExpression("12*34");
    testExpression("100/4");
    testExpression("2+3*4");
    testExpression("(2+3)*4");
    testExpression("10-2*3");
    testExpression("(10-2)*3");
    testExpression("1.5+2.5");
    testExpression("3.14*2");
    testExpression("-5+10");
    testExpression("-(5+3)");
    testExpression("2*3+4*5");
    testExpression("(1+2)*(3+4)");
    testExpression("100/(2+3)");

    // 交互式输入
    char input[256];
    printf("请输入表达式 (输入 'quit' 退出):\n");

    while (1)
    {
        printf("> ");
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }

        // 去掉换行符
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "quit") == 0)
        {
            break;
        }

        if (strlen(input) == 0)
        {
            continue;
        }

        double result = evaluateExpression(input);
        printf("结果: %.2f\n", result);
    }

    printf("程序结束。\n");
    return 0;
}