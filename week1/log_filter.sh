#!/bin/sh

# log_filter.sh - 日志过滤与统计工具 (POSIX 兼容版本)
# 支持按关键字、日期过滤日志并输出统计信息

# 默认值
LOG_FILE=""
KEYWORD=""
START_DATE=""
END_DATE=""
OUTPUT_FILE=""
CASE_SENSITIVE=false
STATS_ONLY=false
VERBOSE=false

# 显示帮助信息
show_help() {
    printf "日志过滤与统计工具\n"
    printf "用法: %s [选项] <日志文件>\n" "$0"
    printf "\n"
    printf "选项:\n"
    printf "  -k <关键字>        按关键字过滤日志\n"
    printf "  -s <日期>          开始日期 (格式: YYYY-MM-DD 或 YYYY-MM-DD HH:MM:SS)\n"
    printf "  -e <日期>          结束日期 (格式: YYYY-MM-DD 或 YYYY-MM-DD HH:MM:SS)\n"
    printf "  -o <文件>          输出到文件\n"
    printf "  -c                 区分大小写搜索\n"
    printf "  -S                 仅显示统计信息\n"
    printf "  -v                 详细输出\n"
    printf "  -h                 显示帮助信息\n"
    printf "\n"
    printf "示例:\n"
    printf "  %s -k \"ERROR\" /var/log/app.log\n" "$0"
    printf "  %s -s \"2024-01-01\" -e \"2024-01-31\" /var/log/app.log\n" "$0"
    printf "  %s -k \"ERROR\" -s \"2024-01-01 10:00:00\" -o filtered.log /var/log/app.log\n" "$0"
    printf "  %s -S /var/log/app.log\n" "$0"
}

# 解析命令行参数
parse_args() {
    while [ $# -gt 0 ]; do
        case "$1" in
            -k)
                if [ -z "$2" ]; then
                    printf "错误: -k 需要参数\n" >&2
                    exit 1
                fi
                KEYWORD="$2"
                shift 2
                ;;
            -s)
                if [ -z "$2" ]; then
                    printf "错误: -s 需要参数\n" >&2
                    exit 1
                fi
                START_DATE="$2"
                shift 2
                ;;
            -e)
                if [ -z "$2" ]; then
                    printf "错误: -e 需要参数\n" >&2
                    exit 1
                fi
                END_DATE="$2"
                shift 2
                ;;
            -o)
                if [ -z "$2" ]; then
                    printf "错误: -o 需要参数\n" >&2
                    exit 1
                fi
                OUTPUT_FILE="$2"
                shift 2
                ;;
            -c)
                CASE_SENSITIVE=true
                shift
                ;;
            -S)
                STATS_ONLY=true
                shift
                ;;
            -v)
                VERBOSE=true
                shift
                ;;
            -h)
                show_help
                exit 0
                ;;
            -*)
                printf "错误: 未知选项 %s\n" "$1" >&2
                show_help
                exit 1
                ;;
            *)
                LOG_FILE="$1"
                shift
                ;;
        esac
    done
}

# 验证参数
validate_args() {
    if [ -z "$LOG_FILE" ]; then
        printf "错误: 请指定日志文件\n" >&2
        show_help
        exit 1
    fi
    
    if [ ! -f "$LOG_FILE" ]; then
        printf "错误: 日志文件 '%s' 不存在\n" "$LOG_FILE" >&2
        exit 1
    fi
    
    if [ ! -r "$LOG_FILE" ]; then
        printf "错误: 日志文件 '%s' 无法读取\n" "$LOG_FILE" >&2
        exit 1
    fi
}

# 转换日期格式为时间戳
date_to_timestamp() {
    date_str="$1"
    
    # 如果只有日期，添加时间部分
    case "$date_str" in
        [0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9])
            date_str="$date_str 00:00:00"
            ;;
    esac
    
    # 尝试转换为时间戳
    if command -v date >/dev/null 2>&1; then
        # 尝试GNU date格式
        if date -d "$date_str" +%s 2>/dev/null; then
            return 0
        fi
        # 尝试BSD date格式
        if date -j -f "%Y-%m-%d %H:%M:%S" "$date_str" +%s 2>/dev/null; then
            return 0
        fi
    fi
    printf "0"
}

# 从日志行提取时间戳
extract_timestamp() {
    line="$1"
    
    # 使用sed提取时间戳 - ISO格式
    timestamp=$(printf "%s" "$line" | sed -n 's/^\([0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\}[ T][0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\).*/\1/p')
    if [ -n "$timestamp" ]; then
        # 将T替换为空格
        timestamp=$(printf "%s" "$timestamp" | sed 's/T/ /')
        printf "%s" "$timestamp"
        return 0
    fi
    
    # 斜杠格式
    timestamp=$(printf "%s" "$line" | sed -n 's/^\([0-9]\{4\}\/[0-9]\{2\}\/[0-9]\{2\} [0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\).*/\1/p')
    if [ -n "$timestamp" ]; then
        # 将斜杠替换为连字符
        timestamp=$(printf "%s" "$timestamp" | sed 's/\//\-/g')
        printf "%s" "$timestamp"
        return 0
    fi
    
    # 带方括号的格式
    timestamp=$(printf "%s" "$line" | sed -n 's/^\[\([0-9]\{4\}-[0-9]\{2\}-[0-9]\{2\} [0-9]\{2\}:[0-9]\{2\}:[0-9]\{2\}\)\].*/\1/p')
    if [ -n "$timestamp" ]; then
        printf "%s" "$timestamp"
        return 0
    fi
    
    printf ""
}

# 字符串包含检查 (POSIX兼容)
string_contains() {
    string="$1"
    substring="$2"
    case_sensitive="$3"
    
    if [ "$case_sensitive" = "true" ]; then
        case "$string" in
            *"$substring"*) return 0 ;;
            *) return 1 ;;
        esac
    else
        # 转换为小写进行比较
        string_lower=$(printf "%s" "$string" | tr '[:upper:]' '[:lower:]')
        substring_lower=$(printf "%s" "$substring" | tr '[:upper:]' '[:lower:]')
        case "$string_lower" in
            *"$substring_lower"*) return 0 ;;
            *) return 1 ;;
        esac
    fi
}

# 数字比较
num_compare() {
    num1="$1"
    op="$2"
    num2="$3"
    
    case "$op" in
        "-lt") [ "$num1" -lt "$num2" ] ;;
        "-gt") [ "$num1" -gt "$num2" ] ;;
        "-eq") [ "$num1" -eq "$num2" ] ;;
        "-le") [ "$num1" -le "$num2" ] ;;
        "-ge") [ "$num1" -ge "$num2" ] ;;
        *) return 1 ;;
    esac
}

# 过滤日志
filter_logs() {
    temp_file=$(mktemp)
    filtered_count=0
    
    if [ "$VERBOSE" = "true" ]; then
        printf "开始过滤日志...\n"
    fi
    
    # 转换日期为时间戳
    start_ts=0
    end_ts=0
    
    if [ -n "$START_DATE" ]; then
        start_ts=$(date_to_timestamp "$START_DATE")
        if [ "$start_ts" = "0" ]; then
            printf "错误: 开始日期格式无效\n" >&2
            exit 1
        fi
    fi
    
    if [ -n "$END_DATE" ]; then
        end_ts=$(date_to_timestamp "$END_DATE")
        if [ "$end_ts" = "0" ]; then
            printf "错误: 结束日期格式无效\n" >&2
            exit 1
        fi
    fi
    
    # 逐行处理日志
    while IFS= read -r line; do
        match=true
        
        # 关键字过滤
        if [ -n "$KEYWORD" ]; then
            if ! string_contains "$line" "$KEYWORD" "$CASE_SENSITIVE"; then
                match=false
            fi
        fi
        
        # 日期过滤
        if [ "$match" = "true" ] && [ -n "$START_DATE$END_DATE" ]; then
            log_timestamp=$(extract_timestamp "$line")
            
            if [ -n "$log_timestamp" ]; then
                log_ts=$(date_to_timestamp "$log_timestamp")
                
                if [ "$log_ts" != "0" ]; then
                    if [ -n "$START_DATE" ] && num_compare "$log_ts" "-lt" "$start_ts"; then
                        match=false
                    fi
                    
                    if [ -n "$END_DATE" ] && num_compare "$log_ts" "-gt" "$end_ts"; then
                        match=false
                    fi
                fi
            fi
        fi
        
        if [ "$match" = "true" ]; then
            printf "%s\n" "$line" >> "$temp_file"
            filtered_count=$((filtered_count + 1))
        fi
        
    done < "$LOG_FILE"
    
    printf "%s" "$temp_file"
    return $filtered_count
}

# 计算文件行数
count_lines() {
    wc -l < "$1" | tr -d ' '
}

# 生成统计信息
generate_stats() {
    filtered_file="$1"
    filtered_count="$2"
    
    printf "=== 日志统计信息 ===\n"
    printf "日志文件: %s\n" "$LOG_FILE"
    printf "总行数: %s\n" "$(count_lines "$LOG_FILE")"
    printf "过滤后行数: %s\n" "$filtered_count"
    
    if [ -n "$KEYWORD" ]; then
        printf "关键字: %s\n" "$KEYWORD"
    fi
    
    if [ -n "$START_DATE" ]; then
        printf "开始日期: %s\n" "$START_DATE"
    fi
    
    if [ -n "$END_DATE" ]; then
        printf "结束日期: %s\n" "$END_DATE"
    fi
    
    if [ -s "$filtered_file" ]; then
        printf "\n=== 日志级别统计 ===\n"
        
        # 统计常见日志级别
        for level in ERROR WARN INFO DEBUG TRACE FATAL; do
            count=$(grep -i "$level" "$filtered_file" | wc -l | tr -d ' ')
            if [ "$count" -gt 0 ]; then
                printf "%s: %s\n" "$level" "$count"
            fi
        done
        
        printf "\n=== 时间范围 ===\n"
        
        # 获取第一行和最后一行的时间戳
        first_line=$(head -n 1 "$filtered_file")
        last_line=$(tail -n 1 "$filtered_file")
        
        first_timestamp=$(extract_timestamp "$first_line")
        last_timestamp=$(extract_timestamp "$last_line")
        
        if [ -n "$first_timestamp" ]; then
            printf "最早时间: %s\n" "$first_timestamp"
        fi
        
        if [ -n "$last_timestamp" ]; then
            printf "最晚时间: %s\n" "$last_timestamp"
        fi
        
        printf "\n=== 前10个最常见的IP地址 ===\n"
        grep -o '[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}' "$filtered_file" | \
        sort | uniq -c | sort -nr | head -10 | \
        while read -r count ip; do
            printf "%s: %s\n" "$ip" "$count"
        done
    fi
}

# 主函数
main() {
    parse_args "$@"
    validate_args
    
    # 过滤日志
    filtered_file=$(filter_logs)
    filtered_count=$?
    
    # 生成统计信息
    generate_stats "$filtered_file" "$filtered_count"
    
    # 输出过滤结果
    if [ "$STATS_ONLY" = "false" ]; then
        printf "\n=== 过滤结果 ===\n"
        
        if [ -n "$OUTPUT_FILE" ]; then
            cp "$filtered_file" "$OUTPUT_FILE"
            printf "结果已保存到: %s\n" "$OUTPUT_FILE"
        else
            printf "\n"
            cat "$filtered_file"
        fi
    fi
    
    # 清理临时文件
    rm -f "$filtered_file"
    
    if [ "$VERBOSE" = "true" ]; then
        printf "处理完成!\n"
    fi
}

# 运行主函数
main "$@"